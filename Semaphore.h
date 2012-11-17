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
		Semaphore();
		~Semaphore();
		void Increment();
		void Decrement();
	};
};

#endif
