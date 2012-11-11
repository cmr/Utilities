#include "Misc.h"

void Utilities::Misc::Base64Encode(uint8* data, uint32 dataLength, int8** result, uint32* resultLength) {
	static char* characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static int32 mask = 0x3F;
	uint32 i, j, left;

	union {
		uint8 bytes[4];
		uint32 block;
	} buffer;

	*resultLength = dataLength + dataLength / 3 + (int32)(dataLength % 3 != 0);
	if (*resultLength % 4)
		*resultLength += 4 - *resultLength % 4;

	*result = new int8[*resultLength];

	for(i = 0, j = 0, left = dataLength; i < dataLength; i += 3, j += 4, left -= 3) {
		buffer.bytes[2] = data[i];
		if(left > 1) {
			buffer.bytes[1] = data[i + 1];
			if(left > 2)
				buffer.bytes[0] = data[i + 2];
			else
				buffer.bytes[0] = 0;
		}
		else {
			buffer.bytes[1] = 0;
			buffer.bytes[0] = 0;
		}

		(*result)[j] = characters[(buffer.block >> 18 ) & mask];
		(*result)[j + 1] = characters[(buffer.block >> 12 ) & mask];
		if(left > 1) {
			(*result)[ j + 2 ] = characters[(buffer.block >> 6) & mask];
			if( left > 2 )
				(*result)[j + 3] = characters[buffer.block & mask];
			else
				(*result)[j + 3] = '=';
		}
		else {
			(*result)[j + 2] = '=';
			(*result)[j + 3] = '=';
		}
	}
}

bool Utilities::Misc::IsStringUTF8(std::string str) {
	uint16 length;
	uint8 byte;
	uint8 bytesToFind;
	uint8 i;
	const int8* bytes;

	length = (uint16)str.size();
	byte = 0;
	bytesToFind = 0;
	bytes = str.data();

	for (byte = bytes[i = 0]; i < length; byte = bytes[++i])
		if (bytesToFind == 0)
			if ((byte >> 1) == 126)
				bytesToFind = 5;
			else if ((byte >> 2) == 62)
				bytesToFind = 4;
			else if ((byte >> 3) == 30)
				bytesToFind = 3;
			else if ((byte >> 4) == 14)
				bytesToFind = 2;
			else if ((byte >> 5) == 6)
				bytesToFind = 1;
			else if ((byte >> 7) == 0) 
				;
			else
				return false;
		else
			if ((byte >> 6) == 2)
				bytesToFind--;
			else
				return false;

	return true;
}

void Utilities::Misc::MemoryBlockCopy(uint8* source, uint8* destination, uint64 amount) {
	uint8 i;

	if (amount > 0) {
		for (; amount >= 8; amount -= 8) 
			*(uint64*)(destination + amount - 8) = *(uint64*)(source + amount - 8);

		for (i = 0; i < amount; i++)
			*(uint8*)(destination + i) = *(source + i);
	}
}

bool Utilities::Misc::MemoryCompare(uint8* blockA, uint8* blockB, uint64 lengthA, uint64 lengthB) {
	if (lengthA != lengthB)
		return false;

	for (; lengthA > 0; lengthA--)
		if (blockA[lengthA - 1] != blockB[lengthA - 1])
			return false;

	return true;
}
