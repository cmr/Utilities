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
		class exported Client {
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
			
			void WebSocketDoHandshake();
			void WebSocketOnReceive();
			bool WebSocketSend(uint8* data, uint16 length, uint8 opCode);
			void WebSocketClose(uint16 code, bool callDisconnect);

		public:
			void* State;
			
			Client(TCPServer* server, Socket* connection);
			~Client();

			bool Send(uint8* buffer, uint16 length);
			void Disconnect();
			void ReadMessage();
		};

		typedef void* (*OnConnectCallback)(TCPServer::Client* client, uint8 clientAddress[Socket::ADDRESS_LENGTH]); /* return a pointer to a state object passed in to OnReceive */
		typedef void (*OnDisconnectCallback)(void* state);
		typedef void (*OnReceiveCallback)(void* state, uint8* buffer, uint16 length);

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
		exported TCPServer::TCPServer();
		exported TCPServer::~TCPServer();
				 
		exported bool Listen(int8* port, bool isWebSocket, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback, OnDisconnectCallback disconnectCallback);
		exported void Shutdown();
	};
};

#endif
