#include "TCPServer.h"

#include <algorithm>

#include "Array.h"
#include "Misc.h"
#include "DataStream.h"

#define WS_HEADER_LINES 25
#define WS_MASK_BYTES 4
#define WS_CLOSE_OPCODE 0x8
#define WS_BINARY_OPCODE 0x2
#define WS_PONG_OPCODE 0xA
#define WS_PING_OPCODE 0x9
#define WS_TEXT_OPCODE 0x1
#define WS_CONTINUATION_OPCODE 0x0

using namespace std;
using namespace Utilities;

static void SocketReadCallback(Socket* socket, void* state) {
	((TCPServer::Client*)state)->ReadMessage();
}

TCPServer::TCPServer() {
	this->IsWebSocket = false;
	this->Active = false;
	this->ConnectCallback = nullptr;
	this->DisconnectCallback = nullptr;
	this->ReceiveCallback = nullptr;
	this->Listener = nullptr;
	this->AcceptWorker = nullptr;
	this->AsyncWorker = nullptr;
}

TCPServer::~TCPServer() {
	this->Shutdown();
}

void TCPServer::AcceptWorkerRun() {
	Socket* acceptedSocket;
	Client* newClient;

	while (this->Active) {
		acceptedSocket = this->Listener->Accept();

		if (acceptedSocket) {
			newClient = new TCPServer::Client(this, acceptedSocket);

			if (this->ConnectCallback)
				newClient->State = this->ConnectCallback(newClient, acceptedSocket->RemoteEndpointAddress);
			
			this->ClientListLock.lock();
			this->ClientList.push_back(newClient);
			this->ClientListLock.unlock();

			this->AsyncWorker->RegisterSocket(acceptedSocket, newClient);
		}
	}
}

bool TCPServer::Listen(int8* port, bool isWebSocket, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback, OnDisconnectCallback disconnectCallback) {
	this->Listener = new Socket(Socket::Families::IPAny, Socket::Types::TCP);
	if (this->Listener->Listen(port)) {
		this->Active = true;
		this->IsWebSocket = isWebSocket;
		this->ReceiveCallback = receiveCallback;
		this->DisconnectCallback = disconnectCallback;
		this->ConnectCallback = connectCallback;
		this->AcceptWorker = new thread(&TCPServer::AcceptWorkerRun, this);
		this->AsyncWorker = new SocketAsyncWorker(SocketReadCallback);
	}
	else {
		delete this->Listener;
		return false;
	}

	return true;
}

void TCPServer::Shutdown() {
	if (!this->Active)
		return;

	while (!this->ClientList.empty())
		this->ClientList[0]->Disconnect();

	this->Active = false;
	this->IsWebSocket = false;
	this->ReceiveCallback = nullptr;
	this->DisconnectCallback = nullptr;
	this->ConnectCallback = nullptr;

	this->Listener->Close();
	this->AcceptWorker->join();
	
	delete this->Listener;
	delete this->AcceptWorker;
	delete this->AsyncWorker;

	this->Listener = nullptr;
	this->AcceptWorker = nullptr;
	this->AsyncWorker = nullptr;
}

TCPServer::Client::Client(TCPServer* server, Socket* connection) {
	this->Server = server;
	this->Connection = connection;
	this->Active = true;
	this->State = nullptr;
	this->WebSocketReady = false;
	this->WebSocketCloseSent = false;
	this->BytesReceived = 0;
	this->MessageLength = 0;
} 

TCPServer::Client::~Client() {
	this->Disconnect();
} 

bool TCPServer::Client::Send(uint8 const* buffer, uint16 length) {
	assert(buffer != nullptr);
	
	if (this->Active) {
		if (this->Server->IsWebSocket) {
			return this->WebSocketSend(buffer, length, WS_BINARY_OPCODE);
		}
		else {
			if (this->Connection->EnsureWrite((uint8*)&length, sizeof(length), 10) != sizeof(length)) {
				this->Disconnect();
				return false;
			}

			if (this->Connection->EnsureWrite(buffer, length, 10) != length) {
				this->Disconnect();
				return false;
			}
		}
	}
	else {
		return false;
	}

	return true;
}

void TCPServer::Client::AddPart(uint8 const* buffer, uint16 length) {
	assert(buffer != nullptr);
	
	if (this->Active)
		this->MessageParts.push_back(make_pair(buffer, length));
}

bool TCPServer::Client::SendParts() {
	vector<pair<uint8 const*, uint16>>::iterator i;
	uint16 totalLength = 0;

	if (this->Active) {
		if (this->Server->IsWebSocket) {
			return this->WebSocketSendParts();
		}
		else {
			for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++)
				totalLength += i->second;

			if (this->Connection->EnsureWrite((uint8*)&totalLength, sizeof(totalLength), 10) != sizeof(totalLength)) {
				this->Disconnect();
				return false;
			}
			
			for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++) {
				if (this->Connection->EnsureWrite(i->first, i->second, 10) != i->second) {
					this->Disconnect();
					return false;
				}
			}
		}
	}
	else {
		return false;
	}

	this->MessageParts.clear();

	return true;
}

void TCPServer::Client::Disconnect() {
	vector<TCPServer::Client*>::iterator position;

	if (!this->Active)
		return;

	if (this->WebSocketReady && !this->WebSocketCloseSent && this->Server->IsWebSocket)
		this->WebSocketClose(1001, false);
		
	if (this->Server->DisconnectCallback)
		this->Server->DisconnectCallback(this, this->State);

	this->Server->AsyncWorker->UnregisterSocket(this->Connection);
	this->Connection->Close();

	this->Server->ClientListLock.lock();

	position = find(this->Server->ClientList.begin(), this->Server->ClientList.end(), this);
	if (position != this->Server->ClientList.end())
		this->Server->ClientList.erase(position, position + 1);

	this->Server->ClientListLock.unlock();
	
	delete this->Connection;
	this->Connection = nullptr;
	this->Server = nullptr;
	this->State = nullptr;
	this->Active = false;
	this->MessageParts.clear();
}

void TCPServer::Client::ReadMessage() {
	TCPServer* server;
	uint32 received;
	uint32 excess;
	uint32 messageLength;

	server = this->Server;

	if (server->IsWebSocket) {
		this->WebSocketOnReceive();
		return;
	}

	received = this->Connection->Read(this->Buffer + this->BytesReceived, Client::MESSAGE_MAXSIZE - this->BytesReceived);
	this->BytesReceived += received;

	if (received == 0) {
		this->Disconnect();
		return;
	}

	if (received > Client::MESSAGE_LENGTHBYTES) {
		messageLength = *(uint16*)this->Buffer;
		excess = received - Client::MESSAGE_LENGTHBYTES - messageLength;

		while (excess >= 0 && messageLength != 0) {
			if (server->ReceiveCallback)
				server->ReceiveCallback(this, this->State, this->Buffer + Client::MESSAGE_LENGTHBYTES, messageLength);
			
			if (excess > 0) {
				Misc::MemoryBlockCopy(this->Buffer + this->BytesReceived - excess, this->Buffer, excess);
				this->BytesReceived = excess;
				if (excess > Client::MESSAGE_LENGTHBYTES) {
					messageLength = *(uint16*)this->Buffer;
					excess -= Client::MESSAGE_LENGTHBYTES + messageLength;
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

void TCPServer::Client::WebSocketDoHandshake() {
	int32 i;
	int32 lastOffset;
	int32 nextHeaderLine;
	Array headerLines[WS_HEADER_LINES];
	Array keyAndMagic(60);
	Array response;
	uint8 hash[Cryptography::SHA1_LENGTH];
	int8* base64;
	uint32 base64Length;
	
	this->BytesReceived = this->Connection->Read(this->Buffer + this->BytesReceived, MESSAGE_MAXSIZE - this->BytesReceived);

	if (this->BytesReceived == 0) {
		this->Disconnect();
		return;
	}

	for (i = 0, lastOffset = 0, nextHeaderLine = 0; i < this->BytesReceived && nextHeaderLine < WS_HEADER_LINES; i++) {
		if (this->Buffer[i] == 13 && this->Buffer[i + 1] == 10) {
			headerLines[nextHeaderLine].Write(this->Buffer + lastOffset, 0, i - lastOffset);
			nextHeaderLine++;
			lastOffset = i + 2;
		}
	}
	
	for (i = 0; i < nextHeaderLine; i++) {
		if (headerLines[i].Size >= 17 && Misc::MemoryCompare((uint8*)"Sec-WebSocket-Key", headerLines[i].Buffer, 17, 17)) {
			keyAndMagic.Write(headerLines[i].Buffer + 19, 0, 24);
			break;
		}
	}
	
	keyAndMagic.Write((uint8*)"258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 24, 36);
	Cryptography::SHA1(keyAndMagic.Buffer, 60, hash);
	Misc::Base64Encode(hash, 20, &base64, &base64Length);

	response.Write((uint8*)"HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ", 0, 97);
	response.Write((uint8*)base64, 97, base64Length);
	response.Write((uint8*)"\r\n\r\n", 97 + base64Length, 4);
	
	if (this->Connection->EnsureWrite(response.Buffer, 97 + 4 + base64Length, 10) != 97 + 4 + base64Length) {
		this->Disconnect();
		return;
	}

	this->WebSocketReady = true;
	this->BytesReceived = 0;
	
	delete[] base64;
}

void TCPServer::Client::WebSocketOnReceive() {
	uint8 bytes[2];
	uint8 FIN;
	uint8 RSV1;
	uint8 RSV2;
	uint8 RSV3;
	uint16 opCode;
	uint8 mask;
	uint16 length;
	uint32 i;
	uint32 received;
	uint8 headerEnd;
	uint8 maskBuffer[WS_MASK_BYTES];
	uint8* payloadBuffer;
	uint8* dataBuffer; //used for websocket's framing. The unmasked data from the current frame is copied to the start of Client.Buffer. Client.MessageLength is then used to point to the end of the data copied to the start of Client.Buffer.

	if (this->WebSocketReady == false) {
		this->WebSocketDoHandshake();
		return;
	}

	dataBuffer = this->Buffer + this->MessageLength;
	received = this->Connection->Read(dataBuffer + this->BytesReceived, MESSAGE_MAXSIZE - this->BytesReceived - this->MessageLength);
	this->BytesReceived += received;
	
	if (this->BytesReceived == 0) {
		this->WebSocketReady = false;
		this->Disconnect();
		return;
	}

	while (this->BytesReceived > 0) {
		if (this->BytesReceived >= 2) {
			bytes[0] = dataBuffer[0];
			bytes[1] = dataBuffer[1];

			headerEnd = 2;
		
			FIN = bytes[0] >> 7 & 0x1;
			RSV1 = bytes[0] >> 6 & 0x1;
			RSV2 = bytes[0] >> 5 & 0x1;
			RSV3 = bytes[0] >> 4 & 0x1;
			opCode = bytes[0] & 0xF;

			mask = bytes[1] >> 7 & 0x1;
			length = bytes[1] & 0x7F;

			if (mask == false || RSV1 || RSV2 || RSV3) {
				this->WebSocketClose(1002, true);
				return;
			}

			if (length == 126) {
				if (this->BytesReceived < 4)
					return;
				length = Socket::NetworkToHostShort((uint16)*dataBuffer + 2);
				headerEnd += 2;
			}
			else if (length == 127) { //we don't support messages this big
				this->WebSocketClose(1009, true);
				return;	
			}

			switch (opCode) { //ping is handled by the application, not websocket
				case WS_TEXT_OPCODE: 
					this->WebSocketClose(1003, true);
					return;
				case WS_CLOSE_OPCODE: 
					this->WebSocketClose(1000, true);
					return;
				case WS_PONG_OPCODE:
					headerEnd += mask ? WS_MASK_BYTES : 0;
					this->BytesReceived -= headerEnd;
					dataBuffer = this->Buffer + headerEnd;
					continue;
				case WS_PING_OPCODE: 
					if (length <= 125) {
						bytes[0] = 128 | WS_PONG_OPCODE;  
						if (this->Connection->EnsureWrite(bytes, 2, 10) != 2 || this->Connection->EnsureWrite(dataBuffer + headerEnd, length + 4, 10) != length + 4) {
							this->WebSocketReady = false;
							this->Disconnect();
							return;
						} 
						headerEnd += mask ? WS_MASK_BYTES : 0;
						this->BytesReceived -= headerEnd;
						dataBuffer = this->Buffer + headerEnd;
						continue;
					}
					else {
						this->WebSocketClose(1009, true);
						return;
					}
				case WS_CONTINUATION_OPCODE:
				case WS_BINARY_OPCODE:  
					Misc::MemoryBlockCopy(dataBuffer + headerEnd, maskBuffer, WS_MASK_BYTES);
					headerEnd += WS_MASK_BYTES;
					payloadBuffer = dataBuffer + headerEnd;
					
					this->BytesReceived -= length + headerEnd;
					for (i = 0; i < length; i++)
						dataBuffer[i] = payloadBuffer[i] ^ maskBuffer[i % WS_MASK_BYTES];

					if (FIN) {
						if (this->Server->ReceiveCallback)
							this->Server->ReceiveCallback(this, this->State, this->Buffer, this->MessageLength + length);
						Misc::MemoryBlockCopy(this->Buffer + length + headerEnd, this->Buffer, this->BytesReceived);
						dataBuffer = this->Buffer;
						this->MessageLength = 0;
					}
					else {
						this->MessageLength += length;
						dataBuffer = this->Buffer + this->MessageLength;
						Misc::MemoryBlockCopy(payloadBuffer + length, dataBuffer, this->BytesReceived);
					}
					continue;
				default:
					this->WebSocketClose(1002, true);
					return;
			}
		}
	}
}

bool TCPServer::Client::WebSocketSend(uint8 const* data, uint16 length, uint8 opCode) {
	uint8 bytes[4];
	uint8 sendLength;

	bytes[0] = 128 | opCode;
	sendLength = sizeof(length);

	if (length <= 125) {
		bytes[1] = (uint8)length;
		*(uint16*)(bytes + 2) = 0;
	}
	else if (length <= 65536) {
		bytes[1] = 126;
		sendLength += 2;
		*(uint16*)(bytes + 2) = Socket::HostToNetworkShort(length);
	}
	else { // we dont support longer messages
		this->WebSocketClose(1004, true);
		return false;	
	}

	if (this->Connection->EnsureWrite(bytes, sendLength, 10) != sendLength) 
		goto sendFailed;
	if (this->Connection->EnsureWrite(data, length, 10) != length)
		goto sendFailed;

	return true;
sendFailed:
	this->WebSocketCloseSent = true;
	this->Disconnect();
	return false;
}

bool TCPServer::Client::WebSocketSendParts() {
	uint8 bytes[4];
	uint8 sendLength;
	vector<pair<uint8 const*, uint16>>::iterator i;
	uint16 totalLength = 0;
	
	for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++)
		totalLength += i->second;

	bytes[0] = 128 | WS_BINARY_OPCODE;
	sendLength = sizeof(totalLength);

	if (totalLength <= 125) {
		bytes[1] = (uint8)totalLength;
		*(uint16*)(bytes + 2) = 0;
	}
	else if (totalLength <= 65536) {
		bytes[1] = 126;
		sendLength += 2;
		*(uint16*)(bytes + 2) = Socket::HostToNetworkShort(totalLength);
	}
	else { // we dont support longer messages
		this->WebSocketClose(1004, true);
		return false;	
	}

	if (this->Connection->EnsureWrite(bytes, sendLength, 10) != sendLength) 
		goto sendFailed;
		
	for (i = this->MessageParts.begin(); i != this->MessageParts.end(); i++)
		if (this->Connection->EnsureWrite(i->first, i->second, 10) != i->second)
			goto sendFailed;

	this->MessageParts.clear();

	return true;
sendFailed:
	this->WebSocketCloseSent = true;
	this->Disconnect();
	return false;
}

void TCPServer::Client::WebSocketClose(uint16 code, bool callDisconnect) {
	this->WebSocketCloseSent = true;
	this->WebSocketReady = false;

	this->WebSocketSend((uint8*)&code, sizeof(uint16), WS_CLOSE_OPCODE);

	if (callDisconnect)
		this->Disconnect();
}