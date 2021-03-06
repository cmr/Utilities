#include "TCPClient.h"
#include "Misc.h"
#include <mutex>

using namespace Utilities;
using namespace std;

static SocketAsyncWorker* asyncWorker = nullptr;
static mutex asyncWorkerLock;

static void SocketReadCallback(Socket* socket, void* state) {
	((TCPClient*)state)->ReadMessage();
}

void TCPClient::ReadMessage() {
	uint32 received;
	uint32 excess;
	uint32 messageLength;

	received = this->Server->Read(this->Buffer + this->BytesReceived, TCPClient::MESSAGE_MAXSIZE - this->BytesReceived);
	this->BytesReceived += received;

	if (received == 0) {
		if (this->ServerDisconnectCallback)
			this->ServerDisconnectCallback(this, this->State);
		this->Disconnect();
		return;
	}

	if (received > TCPClient::MESSAGE_LENGTHBYTES) {
		messageLength = *(uint16*)this->Buffer;
		excess = received - TCPClient::MESSAGE_LENGTHBYTES - messageLength;

		while (excess >= 0 && messageLength != 0) {
			if (this->ReceiveCallback)
				this->ReceiveCallback(this, this->State, this->Buffer + TCPClient::MESSAGE_LENGTHBYTES, messageLength);
			
			if (excess > 0) {
				Misc::MemoryBlockCopy(this->Buffer + this->BytesReceived - excess, this->Buffer, excess);
				this->BytesReceived = excess;
				if (excess > TCPClient::MESSAGE_LENGTHBYTES) {
					messageLength = *(uint16*)this->Buffer;
					excess -= TCPClient::MESSAGE_LENGTHBYTES + messageLength;
				}
				else {
					messageLength = 0;
				}
			}
			else {
				this->BytesReceived = 0;
				messageLength = 0;
			}
		}
		if (messageLength == 0)
			this->BytesReceived = 0;
	}
}

TCPClient::TCPClient() {
	this->BytesReceived = 0;
	this->State = nullptr;
	this->ReceiveCallback = nullptr;
	this->ServerDisconnectCallback = nullptr;
	this->Connected = false;
	this->Server = new Socket(Socket::Families::IPAny, Socket::Types::TCP);
}

TCPClient::~TCPClient() {
	this->Disconnect();
}

bool TCPClient::Connect(int8* address, int8* port, OnReceiveCallback receiveCallback, OnServerDisconnectCallback serverDisconnectCallback, void* state) {
	if (!this->Connected && this->Server->Connect(address, port)) {
		this->ReceiveCallback = receiveCallback;
		this->ServerDisconnectCallback = serverDisconnectCallback;
		this->State = state;
		this->Connected = true;
		
		asyncWorkerLock.lock();

		if (asyncWorker == nullptr)
			asyncWorker = new SocketAsyncWorker(SocketReadCallback);

		asyncWorker->RegisterSocket(this->Server, this);

		asyncWorkerLock.unlock();
	}
	else {
		return false;
	}

	return true;
}

bool TCPClient::Send(uint8 const* buffer, uint16 length) {
	assert(buffer != nullptr);

	if (!this->Connected)
		return false;
		
	if (this->Server->EnsureWrite((uint8*)&length, sizeof(length), 10) != sizeof(length)) {
		this->Disconnect();
		return false;
	}

	if (this->Server->EnsureWrite(buffer, length, 10) != length) {
		this->Disconnect();
		return false;
	}

	return true;
}

void TCPClient::AddPart(uint8 const* buffer, uint16 length) {
	assert(buffer != nullptr);
	
	if (this->Connected)
		this->MessageParts.push_back(make_pair(buffer, length));
}

bool TCPClient::SendParts() {
	vector<pair<uint8 const*, uint16>>::iterator i;
	uint16 totalLength = 0;

	if (!this->Connected)
		return false;

	for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++)
		totalLength += i->second;

	if (this->Server->EnsureWrite((uint8*)&totalLength, sizeof(totalLength), 10) != sizeof(totalLength)) {
		this->Disconnect();
		return false;
	}
			
	for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++) {
		if (this->Server->EnsureWrite(i->first, i->second, 10) != i->second) {
			this->Disconnect();
			return false;
		}
	}

	this->MessageParts.clear();

	return true;
}

void TCPClient::Disconnect() {
	if (this->Connected) {
		
		asyncWorkerLock.lock();

		asyncWorker->UnregisterSocket(this->Server);

		asyncWorkerLock.unlock();

		this->Server->Close();
		delete this->Server;
		this->Server = nullptr;
		this->State = nullptr;
		this->ReceiveCallback = nullptr;
		this->ServerDisconnectCallback = nullptr;
		this->Connected = false;
		this->MessageParts.clear();	
	}
}

void TCPClient::Cleanup() {
	asyncWorkerLock.lock();

	if (asyncWorker != nullptr)
		delete asyncWorker;

	asyncWorkerLock.unlock();
}

