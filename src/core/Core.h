#pragma once

// Just to be safe for now
#if !defined(IP_PLATFORM_LINUX) && !defined(__x86_64__)
    #error Platform not supported
#endif


#if defined(__GNUC__)
	#if defined(__clang__)
		#define IP_COMPILER_CLANG
	#else
		#define IP_COMPILER_GCC
	#endif
#elif defined(_MSC_VER)
	#define IP_COMPILER_MSVC
#endif


#if defined(__GNUC__)
	#define IP_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
	#define IP_FORCE_INLINE __forceinline
#else
	#define IP_FORCE_INLINE inline
#endif


#if defined(IP_PLATFORM_LINUX)
    #include <signal.h>
    #define IP_DEBUGBREAK() raise(SIGTRAP)
#endif
