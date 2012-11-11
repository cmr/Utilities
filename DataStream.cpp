#include "DataStream.h"

using namespace Utilities;

DataStream::DataStream() {
	this->Cursor = 0;
	this->FurthestWrite = 0;
	this->IsEOF = false;
	this->Allocation = DataStream::MINIMUM_SIZE;
	this->Buffer = new uint8[this->Allocation];
}

DataStream::~DataStream() {
	this->Cursor = 0;
	this->FurthestWrite = 0;
	this->IsEOF = true;
	delete this->Buffer;
}

void DataStream::Resize(uint64 newsize) {
	uint64 actualsize;
	uint8* newData;

	actualsize = DataStream::MINIMUM_SIZE;
	while (actualsize < newsize)
		actualsize *= 2;

	if (actualsize != this->Allocation) {
		newData = new uint8[actualsize];
		Misc::MemoryBlockCopy(this->Buffer, newData, newsize);
		delete this->Buffer;
		this->Buffer = newData;
	}
	
	this->Allocation = actualsize;
}

bool DataStream::Seek(uint64 position) {
	if (position > this->FurthestWrite)
		return false;

	this->Cursor = position;
	this->IsEOF = false;

	return true;
}

void DataStream::Write(uint8* data, uint64 count) {
	assert(data != nullptr);

	if (this->Cursor + count >= this->Allocation)
		this->Resize(this->Cursor + count);

	Misc::MemoryBlockCopy(data, this->Buffer + this->Cursor, count);

	this->Cursor += count;

	if (this->Cursor > this->FurthestWrite)
		this->FurthestWrite = this->Cursor;
}

void DataStream::WriteString(std::string toWrite) {
	uint16 size;

	size = (uint16)toWrite.size();

	this->Write((uint8*)&size, sizeof(size));
	this->Write((uint8*)toWrite.data(), size);
}

uint8* DataStream::Read(uint64 count) {
	uint8* result;

	result = this->Buffer + this->Cursor;
	this->Cursor += count;
	this->IsEOF = this->Cursor >= this->FurthestWrite;

	return result;
}

std::string DataStream::ReadString() {
	uint16 length;
	std::string string;
	int8* stringData;

	if (this->Cursor + sizeof(uint16) <= this->FurthestWrite) {
		length = *(uint16*)this->Read(2);

		if (this->Cursor + length <= this->FurthestWrite) {
			stringData = (int8*)this->Read(length);
			string = std::string(stringData, length);
		}
		else {
			this->IsEOF = true;
			this->Cursor -= sizeof(uint16);
		}
	}
	else
		this->IsEOF = true;

	return string;
}
