#ifndef INCLUDE_UTILITIES_COMMON
#define INCLUDE_UTILITIES_COMMON

#ifdef _WIN64
	#define WINDOWS
	#define exported  __declspec(dllexport)
	#define _CRT_SECURE_NO_WARNINGS
#elif __unix__
	#define POSIX
	#define exported __attribute__((visibility ("default")))
#endif

typedef long long int64;
typedef int int32;
typedef short int16;
typedef char int8;
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef double float64;
typedef float float32;

#ifdef NDEBUG
	#define assert(expression) ((void)0)
#else
	#define assert(expression) (void)( (!!(expression)) || true) /*(logger(#expression, __FILE__, __LINE__), 0) ) we need to make logger*/
#endif

#endif
