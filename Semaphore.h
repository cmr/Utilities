#ifndef INCLUDE_UTILITIES_SEMAPHORE
#define INCLUDE_UTILITIES_SEMAPHORE

#include "Common.h"

#ifdef POSIX
	#include <semaphore.h>
#endif

namespace Utilities {
	class exported Semaphore {
		#ifdef WINDOWS
			void* BaseSemaphore;
		#elif defined POSIX
			sam_t* BaseSemaphore;
		#endif
	public:
		enum class DecrementResult {
			Success,
			TimedOut
		};

		Semaphore();
		~Semaphore();
		void Increment();
		enum DecrementResult Decrement(uint64 timeout);
	};
};

#endif
