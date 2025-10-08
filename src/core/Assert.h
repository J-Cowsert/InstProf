#pragma once

#include "Core.h"
#include "Log.h"


// TODO: need to setup debug/release builds - asserts should only be active during debug builds


#define IP_ASSERT(x, ...) { if(!(x)) { IP_ERROR("Assertion Failed: {}", __VA_ARGS__); IP_DEBUGBREAK(); } }
