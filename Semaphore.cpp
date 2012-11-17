#include "Semaphore.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined POSIX
	#include <errno.h>
#endif

using namespace Utilities;

Semaphore::Semaphore() {
#ifdef WINDOWS
	this->BaseSemaphore = CreateSemaphore(NULL, 0, 4294967295, NULL);
#elif defined POSIX
	this->BaseSemaphore = new sem_t;
	sem_init(this->BaseSemaphore, 0 /* shared between threads */, 0);
#endif
}

Semaphore::~Semaphore() {
#ifdef WINDOWS
	CloseHandle(this->BaseSemaphore);
#elif defined POSIX
	sem_destroy(this->BaseSemaphore);
	delete this->BaseSemaphore;
#endif
}

void Semaphore::Increment() {
#ifdef WINDOWS
	ReleaseSemaphore(this->BaseSemaphore, 1, NULL);
#elif defined POSIX
	sem_post(this->BaseSemaphore);
#endif
}

void Semaphore::Decrement() {
#ifdef WINDOWS
	WaitForSingleObject(this->BaseSemaphore, INFINITE);
#elif defined POSIX
	while (sem_wait(this->BaseSemaphore) == -1 && errno == EINTR);
#endif
}
