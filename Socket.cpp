#include "Socket.h"
#include "Misc.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define FD_SETSIZE 1024
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <Windows.h>

	static bool winsockInitialized = false;
#elif defined POSIX
	#include <sys/select.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdio.h>
	#include <string.h>
#endif

using namespace Utilities;
using namespace std;

Socket::Socket(Families family, Types type) {
	this->Family = family;
	this->Type = type;
	this->Connected = false;
	this->LastError = 0;
}

Socket::~Socket() {
	if (this->Connected)
		this->Close();
}

bool Socket::PrepareRawSocket(int8* address, int8* port, bool willListenOn, void** addressInfo) {
	addrinfo serverHints;
	addrinfo* serverAddrInfo;

#ifdef WINDOWS
	SOCKET rawSocket;

	if (!winsockInitialized) {
		WSADATA startupData;
		WSAStartup(514, &startupData);
		winsockInitialized = true;
	}
#elif defined POSIX
	int rawSocket;
#endif

	memset(&serverHints, 0, sizeof(addrinfo));

	switch (this->Family) {
		case Socket::Families::IPV4: serverHints.ai_family = AF_INET; break;
		case Socket::Families::IPV6: serverHints.ai_family = AF_INET6; break;
		case Socket::Families::IPAny: serverHints.ai_family = AF_UNSPEC; break;
		default: return false;
	}

	switch (this->Type) {
		case Socket::Types::TCP: serverHints.ai_socktype = SOCK_STREAM; break;
		default: return false;
	}

	if (willListenOn)
		serverHints.ai_flags = AI_PASSIVE;
		
	if (getaddrinfo(address, port, &serverHints, &serverAddrInfo) != 0) {
		return false;
	}

	if (serverAddrInfo == nullptr) {
		return false;
	}
	
	rawSocket = socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype, serverAddrInfo->ai_protocol);
#ifdef WINDOWS
	if (rawSocket == INVALID_SOCKET) {
		closesocket(rawSocket); // It might be invalid, who cares?
		freeaddrinfo(serverAddrInfo);
		return false;
	}
#elif defined POSIX
	if (rawSocket == -1) {
		close(rawSocket);
		freeaddrinfo(serverAddrInfo);
		return false;
	}
#endif

	this->RawSocket = rawSocket;
	*(addrinfo**)addressInfo = serverAddrInfo;

	return true;
}

bool Socket::Connect(int8* address, int8* port) {
	addrinfo* serverAddrInfo;
	
	if (!this->PrepareRawSocket(address, port, false, (void**)&serverAddrInfo)) {
		return nullptr;
	}
	if (connect(this->RawSocket, serverAddrInfo->ai_addr, (int)serverAddrInfo->ai_addrlen) != 0) {
		goto error;
	}
	
	freeaddrinfo(serverAddrInfo);

	this->Connected = true;

	return true;

error:
#ifdef WINDOWS
	closesocket(this->RawSocket); // It might be invalid, who cares?
#elif defined POSIX
	close(this->RawSocket);
#endif
	freeaddrinfo(serverAddrInfo);

	return false;
}

bool Socket::Listen(int8* port) {
	struct addrinfo* serverAddrInfo;

	if (!this->PrepareRawSocket(nullptr, port, false, (void**)&serverAddrInfo)) {
		return false;
	}

	if (::bind(this->RawSocket, serverAddrInfo->ai_addr, (int)serverAddrInfo->ai_addrlen) != 0) {
		goto error;
	}

	if (listen(this->RawSocket, SOMAXCONN) != 0) {
		goto error;
	}
	
	freeaddrinfo(serverAddrInfo);

	this->Connected = true;

	return true;

error:
#ifdef WINDOWS
	closesocket(this->RawSocket); // It might be invalid, who cares?
#elif defined POSIX
	close(this->RawSocket);
#endif
	freeaddrinfo(serverAddrInfo);

	return false;
}

Socket* Socket::Accept() {
	Socket* socket;
	sockaddr_storage remoteAddress;
	sockaddr_in* ipv4Address;
	sockaddr_in6* ipv6Address;
	int addressLength = sizeof(remoteAddress);

#ifdef WINDOWS
	SOCKET rawSocket;
	
	rawSocket = accept((SOCKET)this->RawSocket, (sockaddr*)&remoteAddress, &addressLength);
	if (rawSocket == INVALID_SOCKET) {
		return nullptr;
	}

#elif defined POSIX
	int rawSocket;

	rawSocket = accept(this->RawSocket, (sockaddr*)&remoteAddress, (socklen_t*)&addressLength);
	if (rawSocket == -1) {
		return nullptr;
	}

#endif

	socket = new Socket(this->Family, this->Type);
	socket->RawSocket = rawSocket;
	socket->Connected = true;

	memset(socket->RemoteEndpointAddress, 0, sizeof(socket->RemoteEndpointAddress));

	if (remoteAddress.ss_family == AF_INET) {
		ipv4Address = (sockaddr_in*)&remoteAddress;
		memset(socket->RemoteEndpointAddress, 0, 10); //to copy the ipv4 address in ipv6 mapped format
		memset(socket->RemoteEndpointAddress + 10, 1, 2);
#ifdef WINDOWS
		memcpy(socket->RemoteEndpointAddress + 12, (uint8*)&ipv4Address->sin_addr.S_un.S_addr, 4);
#elif defined POSIX
		memcpy(socket->RemoteEndpointAddress + 12, (uint8*)&ipv4Address->sin_addr.s_addr, 4);
#endif
	}
	else if (remoteAddress.ss_family == AF_INET6) {
		ipv6Address = (sockaddr_in6*)&remoteAddress;
#ifdef WINDOWS
		memcpy(socket->RemoteEndpointAddress, ipv6Address->sin6_addr.u.Byte, sizeof(ipv6Address->sin6_addr.u.Byte));
#elif defined POSIX
		memcpy(socket->RemoteEndpointAddress, ipv6Address->sin6_addr.s6_addr, sizeof(ipv6Address->sin6_addr.s6_addr));
#endif
	}

	return socket;
}

void Socket::Close() {
	this->Connected = false;
#ifdef WINDOWS
	shutdown((SOCKET)this->RawSocket, SD_BOTH);
	closesocket((SOCKET)this->RawSocket);
	this->RawSocket = INVALID_SOCKET;
#elif defined POSIX
	shutdown(this->RawSocket, SHUT_RDWR);
	close(this->RawSocket);
	this->RawSocket = -1;
#endif
}

uint32 Socket::Read(uint8* buffer, uint32 bufferSize) {
	int32 received;

	assert(buffer != nullptr);

#ifdef WINDOWS
	received = recv((SOCKET)this->RawSocket, (int8* const)buffer, bufferSize, 0);
#elif defined POSIX
	received = recv(this->RawSocket, (int8* const)buffer, bufferSize, 0);
#endif

	if (received <= 0)
		return 0;

	return (uint32)received;
}

uint32 Socket::Write(uint8 const* toWrite, uint32 writeAmount) {
	int32 result;

	assert(toWrite != nullptr);

#ifdef WINDOWS
	result = send((SOCKET)this->RawSocket, (int8*)toWrite, writeAmount, 0);
#elif defined POSIX
	result = send(this->RawSocket, (int8*)toWrite, writeAmount, 0);
#endif

	return (uint32)result;
}

uint32 Socket::EnsureWrite(uint8 const* toWrite, uint32 writeAmount, uint8 maxAttempts) {
	uint32 sentSoFar;
	int32 result;
	uint8 tries;
	bool isFirstTry;

	assert(toWrite != nullptr);

	sentSoFar = 0;
	tries = 0;
	isFirstTry = true;

	while (true) {
	
		#ifdef WINDOWS
			result = send((SOCKET)this->RawSocket, (int8*)(toWrite + sentSoFar), writeAmount - sentSoFar, 0);
			if (result != SOCKET_ERROR)
				sentSoFar += result;
		#elif defined POSIX
			result = send(this->RawSocket, (int8*)(toWrite + sentSoFar), writeAmount - sentSoFar, 0);
			if (result != -1)
				sentSoFar += result;
		#endif

		tries++;

		if (sentSoFar == writeAmount || tries == maxAttempts)
			break;

		this_thread::sleep_for(chrono::milliseconds(tries * 50));
	}


	return sentSoFar;
}

uint16 Socket::HostToNetworkShort(uint16 value) {
	return htons(value);
}

uint16 Socket::NetworkToHostShort(uint16 value) {
	return ntohs(value);
}



SocketAsyncWorker::SocketAsyncWorker(ReadCallback callback) {
	#ifdef POSIX
	this->MaxFD = 0;
	#endif
	this->Running = true;
	this->Callback = callback;
	this->Worker = thread(&SocketAsyncWorker::Run, this);
}

SocketAsyncWorker::~SocketAsyncWorker() {
	this->Running = false;
	this->Callback = nullptr;
	this->Worker.join();
}

void SocketAsyncWorker::Run() {
	fd_set readSet;
	int readySockets;
	map<Socket*, void*>::iterator listPosition;
	struct timeval selectTimeout;

	selectTimeout.tv_usec = 250;
	selectTimeout.tv_sec = 0;
	listPosition = this->List.begin();
		
	while (this->Running) {
		FD_ZERO(&readSet);

		this_thread::sleep_for(chrono::milliseconds(25));

		this->ListLock.lock();

		for (uint64 i = 0; i < FD_SETSIZE, listPosition != this->List.end(); i++, listPosition++) {
			if (listPosition->second != nullptr) {
				#ifdef WINDOWS
					FD_SET((SOCKET)listPosition->first->RawSocket, &readSet);
				#elif defined POSIX
					FD_SET(listPosition->first->RawSocket, &readSet);
				#endif
			}
		}

		if (listPosition == this->List.end())
			listPosition = this->List.begin();
			
		#ifdef WINDOWS
			readySockets = select(0, &readSet, nullptr, nullptr, &selectTimeout);
		#elif defined POSIX
			readySockets = select(this->MaxFD + 1, &readSet, nullptr, nullptr, &selectTimeout);
		#endif
		
		for (map<Socket*, void*>::iterator j = this->List.begin(); j != this->List.end() && readySockets != 0; j++, readySockets--) {
			Socket* socket = (*j).first;
			void* state = (*j).second;

			if (socket && state && FD_ISSET(socket->RawSocket, &readSet)) {
				this->Callback(socket, state);
				break;
			}
		}

		this->ListLock.unlock();
	}

	return;
}

void SocketAsyncWorker::RegisterSocket(Socket* socket, void* state) {
	this->ListLock.lock();
	#ifdef POSIX
	this->MaxFD = max(socket->RawSocket, this->MaxFD);
	#endif
	this->List[socket] = state;
	this->ListLock.unlock();
}
	
void SocketAsyncWorker::UnregisterSocket(Socket* socket) {
	this->ListLock.lock();
	
	#ifdef POSIX
	for(map<Socket*, void*>::iterator i = this->List.begin(); i != this->List.end(); i++)
		this->MaxFD = max(i->first->RawSocket, this->MaxFD);
	#endif

	this->List[socket] = nullptr;
	this->ListLock.unlock();
}

