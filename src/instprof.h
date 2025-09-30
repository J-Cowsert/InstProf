#pragma once

// Will other macros be used as inputs?  
#define IP_CONCAT_INNER(a, b) a##b 
#define IP_CONCAT(a, b) IP_CONCAT_INNER(a, b)

// Temp toggle
#ifndef IP_ENABLE
    #define IP_ENABLE 1
#endif


#if IP_ENABLE

    #include "core/Zone.h"
   
    #define IP_NAMED_ZONE(name_literal) instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ instprof::make_callsite(name_literal) }
    #define IP_FUNC_SCOPE() instprof::ZoneScope IP_CONCAT(_ipZone_, __COUNTER__){ instprof::make_callsite(__func__) }

#else 

    #define IP_NAMED_ZONE(name_literal)
    #define IP_FUNC_SCOPE
    
#endif
