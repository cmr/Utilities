#ifndef INCLUDE_UTILITIES_SERVER
#define INCLUDE_UTILITIES_SERVER

#include "Common.h"
#include "Socket.h"
#include "Cryptography.h"
#include <thread>
#include <mutex>
#include <vector>

namespace Utilities {
	class TCPServer {
	public:
		class Client {
			static const uint32 MESSAGE_LENGTHBYTES = 2;
			static const uint32 MESSAGE_MAXSIZE = 65536 + MESSAGE_LENGTHBYTES;

			Socket* Connection;
			bool Active;
			bool WebSocketReady;
			bool WebSocketCloseSent;
			TCPServer* Server;
			uint8 Buffer[Client::MESSAGE_MAXSIZE];
			uint16 BytesReceived;
			uint32 MessageLength; //for websockets only
			std::vector< std::pair<uint8 const*, uint16> > MessageParts;
			
			void WebSocketDoHandshake();
			void WebSocketOnReceive();
			bool WebSocketSend(uint8 const* data, uint16 length, uint8 opCode);
			bool WebSocketSendParts();
			void WebSocketClose(uint16 code, bool callDisconnect);

		public:
			void* State;
			
			Client(TCPServer* server, Socket* connection);
			~Client();

			exported bool Send(uint8 const* buffer, uint16 length);
			exported void AddPart(uint8 const* buffer, uint16 length);
			exported bool SendParts();
			exported void Disconnect();
			exported void ReadMessage();
		};

		typedef void* (*OnConnectCallback)(TCPServer::Client* client, uint8 clientAddress[Socket::ADDRESS_LENGTH]); /* return a pointer to a state object passed in to OnReceive */
		typedef void (*OnDisconnectCallback)(TCPServer::Client* client, void* state);
		typedef void (*OnReceiveCallback)(TCPServer::Client* client, void* state, uint8* buffer, uint16 length);

	private:
		Socket* Listener;
		SocketAsyncWorker* AsyncWorker;
		std::thread* AcceptWorker;
		std::mutex ClientListLock;
		std::vector<TCPServer::Client*> ClientList;
		bool IsWebSocket;
		bool Active;
		OnReceiveCallback ReceiveCallback;
		OnConnectCallback ConnectCallback;
		OnDisconnectCallback DisconnectCallback;

		void AcceptWorkerRun();

	public:
		exported TCPServer();
		exported ~TCPServer();
				 
		exported bool Listen(int8* port, bool isWebSocket, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback, OnDisconnectCallback disconnectCallback);
		exported void Shutdown();
	};
};

#endif
