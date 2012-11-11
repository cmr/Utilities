#ifndef INCLUDE_UTILITIES_CRYPTOGRAPHY
#define INCLUDE_UTILITIES_CRYPTOGRAPHY

#include "Common.h"

namespace Utilities {
	namespace Cryptography {
		static const uint8 SHA512_LENGTH = 64;
		static const uint8 SHA1_LENGTH = 20;
		
		exported void SHA512(uint8* source, uint32 length, uint8 hashOutput[SHA512_LENGTH]);
		exported void SHA1(uint8* source, uint32 length, uint8 hashOutput[SHA1_LENGTH]);
		exported void RandomBytes(uint8* buffer, uint64 count);
		exported uint64 RandomInt64(int64 floor, int64 ceiling);
	};
};

#endif
