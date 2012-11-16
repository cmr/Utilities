#ifndef INCLUDE_UTILITIES_TCPCLIENT
#define INCLUDE_UTILITIES_TCPCLIENT

#include "Common.h"
#include "Socket.h"
#include <vector>

namespace Utilities {
	class TCPClient {
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
		std::vector< std::pair<uint8*, uint16> > MessageParts;

	public:
		exported TCPClient::TCPClient();
		exported TCPClient::~TCPClient();
		 
		exported bool Connect(int8* address, int8* port, OnReceiveCallback receiveCallback, OnServerDisconnectCallback serverDisconnectCallback, void* state);
		exported bool Send(uint8* buffer, uint16 length);
		exported void AddPart(uint8* buffer, uint16 length);
		exported bool SendParts();
		exported void Disconnect();
		exported void ReadMessage();
		exported static void Cleanup();
	};
};

#endif
