#ifndef INCLUDE_UTILITIES_TIME
#define INCLUDE_UTILITIES_TIME

#include "Common.h"

namespace Utilities {

	class exported DateTime {
		uint64 milliseconds;

	public:
		DateTime(uint64 milliseconds);

		static DateTime Now();
	};
};

#endif
