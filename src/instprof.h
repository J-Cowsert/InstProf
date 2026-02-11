#pragma once

// User Toggles
#ifndef IP_ENABLE
    #define IP_ENABLE 1
#endif

#if IP_ENABLE

    #include "core/Zone.h"

    #define IP_NAMED_SCOPE(name_literal) static constinit instprof::CallsiteInfo IP_CONCAT(_ipCs_, __LINE__) = instprof::MakeCallsite((name_literal)); IP__REGISTER_CALLSITE(IP_CONCAT(_ipCs_, __LINE__)); instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ &IP_CONCAT(_ipCs_, __LINE__) }
    #define IP_FUNC_SCOPE() static constinit instprof::CallsiteInfo IP_CONCAT(_ipCs_, __LINE__) = instprof::MakeCallsite(__func__); IP__REGISTER_CALLSITE(IP_CONCAT(_ipCs_, __LINE__)); instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ &IP_CONCAT(_ipCs_, __LINE__) }

#else

    #define IP_NAMED_SCOPE(name_literal)
    #define IP_FUNC_SCOPE()
    
#endif
