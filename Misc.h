#ifndef INCLUDE_UTILITIES_MISC
#define INCLUDE_UTILITIES_MISC

#include "Common.h"
#include <string>

namespace Utilities {
	namespace Misc {
		exported void Base64Encode(const uint8* data, uint32 dataLength, int8** result, uint32* resultLength);
		exported bool IsStringUTF8(std::string str);
		exported void MemoryBlockCopy(const uint8* source, uint8* destination, uint64 amount);
		exported bool MemoryCompare(const uint8* blockA, const uint8* blockB, uint64 lengthA, uint64 lengthB);
	};
};


#endif
