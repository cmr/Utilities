#include "Array.h"
#include "Misc.h"

using namespace Utilities;

Array::Array(uint64 size) {
	uint64 actualsize;

	actualsize = Array::MINIMUM_SIZE;
	while (actualsize < size)
		actualsize *= 2;

	this->Size = size;
	this->Allocation = actualsize;
	this->Buffer = new uint8[actualsize];
}

Array::Array(uint8* data, uint64 size) {
	uint64 actualsize;

	actualsize = MINIMUM_SIZE;
	while (actualsize < size)
		actualsize *= 2;

	this->Size = size;
	this->Allocation = actualsize;
	this->Buffer = new uint8[actualsize];

	Misc::MemoryBlockCopy(data, this->Buffer, size);
}

Array::~Array() {
	this->Size = 0;
	this->Allocation = 0;
	delete this->Buffer;
}

void Array::Resize(uint64 newsize) {
	uint64 actualsize;
	uint8* newData;

	actualsize = Array::MINIMUM_SIZE;
	while (actualsize < newsize)
		actualsize *= 2;

	if (actualsize != this->Allocation) {
		newData = new uint8[actualsize];
		Misc::MemoryBlockCopy(this->Buffer, newData, newsize);
		delete this->Buffer;
		this->Buffer = newData;
	}
	
	this->Size = newsize;
	this->Allocation = actualsize;
}

uint8* Array::Read(uint64 position, uint64 amount) {
	assert(position + amount <= this->Size);

	return this->Buffer + position;
}

void Array::ReadTo(uint64 position, uint64 amount, uint8* targetBuffer) {
	assert(targetBuffer != nullptr);
	assert(position + amount <= this->Size);

	Misc::MemoryBlockCopy(this->Buffer + position, targetBuffer, amount);
}

void Array::Write(uint8* data, uint64 position, uint64 amount) {
	if (position + amount > this->Size)
		this->Resize(position + amount);

	Misc::MemoryBlockCopy(data, this->Buffer + position, amount);
}

void Array::Append(Array& source) {
	this->Resize(this->Size + source.Size);
	this->Write(source.Buffer, this->Size, source.Size);
}
