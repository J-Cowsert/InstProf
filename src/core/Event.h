#pragma once

#include <cstdint>

namespace instprof {

    enum class EventType : uint8_t {
        
        ZoneBegin,
        ZoneEnd 
    };

    // TODO: Pack these for better cache alignment
    struct ZoneBegin {

        int64_t time;
        uint64_t callsiteInfo; // pointer to static callsite metadata
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

}