#ifndef INCLUDE_UTILITIES_ARRAY
#define INCLUDE_UTILITIES_ARRAY

#include "Common.h"

namespace Utilities {
	class exported Array {
	public:
		uint64 Size;
		uint8* Buffer;
		uint64 Allocation;
		
		static const uint32 MINIMUM_SIZE = 32;

		Array(uint64 size = Array::MINIMUM_SIZE);
		Array(uint8* data, uint64 size);
		~Array();

		void Resize(uint64 newSize);
		uint8* Read(uint64 position, uint64 amount);
		void ReadTo(uint64 position, uint64 amount, uint8* targetBuffer);
		void Write(uint8* data, uint64 position, uint64 amount);
		void Append(Array& source);
	};
};

#endif
