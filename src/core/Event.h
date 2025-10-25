#pragma once

#include <cstdint>

namespace instprof {


    enum class EventType : uint8_t {
        
        ZoneBegin,
        ZoneEnd 
    };

    struct ZoneBegin {

        int64_t time;
        uint64_t callsiteInfo; // pointer to static callsite metadata
        uint32_t threadID;
    };

    struct ZoneEnd {

        int64_t time;
        uint32_t threadID;
    };

    struct EventHeader {

        union {
            EventType type;
            uint8_t typeIdx;
        };
    };

    struct EventItem {

        EventHeader header;
        union {
            ZoneBegin zoneBegin;
            ZoneEnd zoneEnd;
        };
    };

}