#pragma once

// Temp toggle
#ifndef IP_ENABLE
    #define IP_ENABLE 1
#endif

#define IP_CONCAT_INNER(a, b) a##b 
#define IP_CONCAT(a, b) IP_CONCAT_INNER(a, b)


#if IP_ENABLE

    #include "core/Zone.h"
   
    #define IP_NAMED_SCOPE(name_literal) static constexpr instprof::CallsiteInfo IP_CONCAT(_ipCs_, __LINE__) = instprof::MakeCallsite((name_literal)); instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ &IP_CONCAT(_ipCs_, __LINE__) }
    #define IP_FUNC_SCOPE() static constexpr instprof::CallsiteInfo IP_CONCAT(_ipCs_, __LINE__) = instprof::MakeCallsite(__func__); instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ &IP_CONCAT(_ipCs_, __LINE__) }

#else 

    #define IP_NAMED_ZONE(name_literal)
    #define IP_FUNC_SCOPE()
    
#endif
