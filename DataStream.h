#ifndef INCLUDE_UTILITIES_DATASTREAM
#define INCLUDE_UTILITIES_DATASTREAM

#include "Common.h"
#include "Misc.h"
#include <cstring>

namespace Utilities {
	class exported DataStream {
		uint64 Allocation;
		uint64 Cursor;
		uint64 FurthestWrite;
		uint8* Buffer;
		
		static const uint64 MINIMUM_SIZE = 32;

		void Resize(uint64 newSize);

	public:
		bool IsEOF;
		
		DataStream();
		DataStream(uint8* exisitingBuffer, uint64 length);
		~DataStream();

		uint8 const* GetBuffer();
		uint64 GetLength();

		bool Seek(uint64 position);
		void Write(uint8* bytes, uint64 count);
		void WriteString(std::string toWrite);
		uint8* Read(uint64 count);
		std::string ReadString();

		template <typename T> void Write(T toWrite) {
			this->Write((uint8*)&toWrite, sizeof(T));
		}

		template <typename T> T Read() {
			return *(T*)this->Read(sizeof(T));
		}
	};
};

#endif
