#ifndef INCLUDE_UTILITIES_SOCKET
#define INCLUDE_UTILITIES_SOCKET

#include "Common.h"
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

namespace Utilities {

	class exported Socket {

	public:	
		#ifdef WINDOWS
			uint64 RawSocket;
		#elif defined POSIX
			int RawSocket;
		#endif

		static const uint16 ADDRESS_LENGTH = 16;

		enum class Types {
			TCP
		};

		enum class Families {
			IPV4,
			IPV6,
			IPAny
		};

		uint8 RemoteEndpointAddress[ADDRESS_LENGTH];
		
		Socket(Families family, Types type);
		~Socket();

		bool Connect(int8* address, int8* port);
		bool Listen(int8* port);
		Socket* Accept();
		void Close();
		uint32 Read(uint8* buffer, uint32 bufferSize);
		uint32 Write(uint8* toWrite, uint32 writeAmount);
		uint32 EnsureWrite(uint8* toWrite, uint32 writeAmount, uint8 maxAttempts);

		static uint16 HostToNetworkShort(uint16 value);
		static uint16 NetworkToHostShort(uint16 value);

	private:
		bool Socket::PrepareRawSocket(int8* address, int8* port, bool willListenOn, void** addressInfo);

		Types Type;
		Families Family;
		bool Connected;
		uint8 LastError;
	};
	
	class  SocketAsyncWorker {
	public:
		typedef void (*ReadCallback)(Socket* socket, void* state);

		exported SocketAsyncWorker(ReadCallback callback);
		exported ~SocketAsyncWorker();
		exported void RegisterSocket(Socket* socket, void* state);
		exported void UnregisterSocket(Socket* socket);

	private:
		ReadCallback Callback;
		std::atomic<bool> Running;
		std::thread Worker;
		std::recursive_mutex ListLock;
		std::map<Socket*, void*> List;

		void Run();
	};

};

#endif

