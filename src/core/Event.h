#pragma once

#include <cstdint>

namespace instprof {

    struct CallsiteInfo;

    enum class EventType : uint8_t {
        
        ZoneBegin,
        ZoneEnd 
    };

    struct ZoneBegin {

        int64_t time;
        CallsiteInfo* callsiteInfo; // pointer to static callsite metadata
        // uint32_t threadID;
    };

    struct ZoneEnd {

        int64_t time;
        // uint32_t threadID;
    };
    
    struct EventTag {

        union {
            EventType type;
            uint8_t typeIdx;
        };
    };

    // Causes misaligned access but improves throughput (tested on x86-64) 
    #pragma pack(push, 1)
    struct EventItem {

        EventTag tag;
        union {
            ZoneBegin zoneBegin;
            ZoneEnd zoneEnd;
        };
    };
    #pragma pack(pop)

}
