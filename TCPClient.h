#ifndef INCLUDE_UTILITIES_TCPCLIENT
#define INCLUDE_UTILITIES_TCPCLIENT

#include "Common.h"
#include "Socket.h"

namespace Utilities {
	class exported TCPClient {
	public:
		typedef void (*OnReceiveCallback)(TCPClient* client, void* state, uint8* buffer, uint16 length);
		typedef void (*OnServerDisconnectCallback)(TCPClient* client, void* state);

	private:
		static const uint32 MESSAGE_LENGTHBYTES = 2;
		static const uint32 MESSAGE_MAXSIZE = 65536 + MESSAGE_LENGTHBYTES;
		
		Socket* Server;
		uint8 Buffer[MESSAGE_MAXSIZE];
		uint16 BytesReceived;
		void* State;
		bool Connected;
		OnReceiveCallback ReceiveCallback;
		OnServerDisconnectCallback ServerDisconnectCallback;

	public:
		TCPClient::TCPClient();
		TCPClient::~TCPClient();

		bool Connect(int8* address, int8* port, OnReceiveCallback receiveCallback, OnServerDisconnectCallback serverDisconnectCallback, void* state);
		bool Send(uint8* buffer, uint16 length);
		void Disconnect();
		void ReadMessage();
		static void Cleanup();
	};
};

#endif
