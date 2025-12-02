#pragma once

#include <cstdint>

namespace instprof {

    enum class EventType : uint8_t {
        
        ZoneBegin,
        ZoneEnd 
    };

    #pragma pack(push, 1)

    struct ZoneBegin {

        int64_t time;
        uintptr_t callsiteInfo; // pointer to static callsite metadata
        uint32_t threadID;
    };

    struct ZoneEnd {

        int64_t time;
        uint32_t threadID;
    };
    

    struct EventTag {

        union {
            EventType type;
            uint8_t typeIdx;
        };
    };

    struct EventItem {

        EventTag tag;
        union {
            ZoneBegin zoneBegin;
            ZoneEnd zoneEnd;
        };
    };
    
    #pragma pack(pop)

}