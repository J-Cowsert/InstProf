#pragma once

#ifdef IP_DEBUG
    #include "Core.h"
    #include "Log.h"
    #define IP_ASSERT(x, ...) { if(!(x)) { IP_ERROR("Assertion Failed: {}", __VA_ARGS__); IP_DEBUGBREAK(); } }
#else
    #define IP_ASSERT(x, ...) // No-op in release mode
#endif
