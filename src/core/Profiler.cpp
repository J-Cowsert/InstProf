#include "Profiler.h"

#include "core/Assert.h"
#include "core/Callsite.h"
#include "core/Event.h"
#include "core/Log.h"
#include "core/System.h"
#include "core/Callsite.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>


#ifndef IP_EXPORT_TRACE 
    #define IP_EXPORT_TRACE 0
#endif


namespace instprof {

    Profiler::Profiler() 
        : m_MainThreadID(GetCurrentThreadID()), m_Epoch(GetTime())      
    { 

        IP_ASSERT(!s_Data, "");

        StartWorkerThread();
    }

    Profiler::~Profiler() {

        EndWorkerThread();
        PrintStatsReport();

        std::cerr << m_PushFailCount << std::endl;
    }
    
    bool Profiler::StartWorkerThread() {

        if (m_Running.load()) return false;
        m_Stop.store(false);
        m_Running.store(true);
        m_Worker = std::thread([this]{ ProcessEvents(); });
        return true;
    }

    void Profiler::EndWorkerThread() {

        if (!m_Running.load()) return;
        m_Stop.store(true);
        if (m_Worker.joinable()) m_Worker.join();
        m_Running.store(false);
    }

    void Profiler::ProcessEvents() {

        std::vector<ThreadEntry*> snapshot;
        EventItem ev;
        const int BATCH_SIZE = 512;


        for (;;) {

            bool workFound = false;

            {
                std::lock_guard<std::mutex> lock(m_RegistrationMutex); // TODO: Avoid this lock with a atomic thread counter. Only take a new snapshot if the count changes
                snapshot = m_ThreadEntries;
            }

            for (auto* entry : snapshot) {

                int batchCounter = 0;

                while (entry->EventQueue.TryPop(ev)) {
                    
                    workFound = true;
                    batchCounter++;
    
                    switch (ev.tag.type) {
    
                        case EventType::ZoneBegin:
                        {
                            auto& tState = entry->State;
                            auto& activeZones = tState.activeZoneStack;
    
                            auto& az = activeZones.emplace_back();
                            az.startTime          = ev.zoneBegin.time;
                            az.callsiteInfo       = ev.zoneBegin.callsiteInfo;
                            az.childInclusiveTime = 0;
                            az.depth              = (uint32_t)activeZones.size() - 1; // 0-based depth
    
                            break;
                        }
    
                        case EventType::ZoneEnd:
                        {
                            auto& tState = entry->State;
                            auto& activeZones = tState.activeZoneStack;

                            if (activeZones.empty()) break; // guard against mismatched begin/end
    
                            auto az = activeZones.back();
                            activeZones.pop_back();
    
                            // auto& rec = tState.completedZones.emplace_back();
                            ZoneRecord rec;
                            rec.startTime     = az.startTime;
                            rec.endTime       = ev.zoneEnd.time;
                            rec.callsiteInfo  = az.callsiteInfo;
                            rec.threadID      = entry->ThreadID; 
                            rec.depth         = activeZones.size();
                            rec.inclusiveTime = rec.endTime - rec.startTime;
                            rec.selfTime      = rec.inclusiveTime - az.childInclusiveTime;
                            
                            if (!activeZones.empty()) {
    
                                activeZones.back().childInclusiveTime += rec.inclusiveTime;
                            }
    
                            // Aggregate Stats — written directly into the callsite
                            {
                                auto* cs = reinterpret_cast<CallsiteInfo*>(rec.callsiteInfo);
                                cs->stats.totalInclusiveTime += rec.inclusiveTime;
                                cs->stats.totalSelfTime      += rec.selfTime;
                                cs->stats.maxInclusiveTime   = std::max(cs->stats.maxInclusiveTime, rec.inclusiveTime);
                                cs->stats.maxSelfTime        = std::max(cs->stats.maxSelfTime, rec.selfTime);
                                cs->stats.callCount++;
                            }
    
                            break;
                        } 
                    }

                    if (batchCounter >= BATCH_SIZE) {

                        batchCounter = 0;
                        break;
                    }
                } 
            }

            if (!workFound) {

                if (m_Stop.load()) break;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                // std::this_thread::yield();
            }
        }
   }

    static int FormatTime(int64_t ns, char* buf, size_t len) {

        if (ns < 1'000)
            return snprintf(buf, len, "%6lld ns", (long long)ns);
        if (ns < 1'000'000)
            return snprintf(buf, len, "%6.1f us", ns / 1e3);
        if (ns < 1'000'000'000)
            return snprintf(buf, len, "%6.1f ms", ns / 1e6);
        return snprintf(buf, len, "%6.3f s ", ns / 1e9);
    }

    void Profiler::PrintStatsReport() {

        CallsiteInfo** begin = &__start_instprof_cs;
        CallsiteInfo** end   = &__stop_instprof_cs;

        struct Entry { CallsiteInfo* cs; };
        std::vector<Entry> entries;

        for (auto** it = begin; it < end; ++it) {
            if (*it && (*it)->stats.callCount > 0)
                entries.push_back({*it});
        }

        if (entries.empty()) return;

        std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
            return a.cs->stats.totalSelfTime > b.cs->stats.totalSelfTime;
        });

        const char* hdr  = "  %-24s %8s   %-10s %-10s %-10s %-10s %-10s %-10s\n";
        const char* row  = "  %-24s %8lu   %-10s %-10s %-10s %-10s %-10s %-10s\n";
        const char* line = "  ──────────────────────────────────────────────────────────────────────────────────────────────────\n";

        fprintf(stderr, "\n%s", line);
        fprintf(stderr, "  instprof — Session Statistics  (sorted by total self time)\n");
        fprintf(stderr, "%s", line);
        fprintf(stderr, hdr,
            "Name", "Calls",
            "Self Tot", "Self Avg", "Self Max",
            "Incl Tot", "Incl Avg", "Incl Max");
        fprintf(stderr, "%s", line);

        uint64_t totalCalls = 0;

        for (auto& [cs] : entries) {
            auto& s = cs->stats;
            totalCalls += s.callCount;

            int64_t avgSelf = s.totalSelfTime      / (int64_t)s.callCount;
            int64_t avgIncl = s.totalInclusiveTime  / (int64_t)s.callCount;

            char st[16], sa[16], sm[16], it[16], ia[16], im[16];
            FormatTime(s.totalSelfTime,      st, sizeof(st));
            FormatTime(avgSelf,              sa, sizeof(sa));
            FormatTime(s.maxSelfTime,        sm, sizeof(sm));
            FormatTime(s.totalInclusiveTime, it, sizeof(it));
            FormatTime(avgIncl,              ia, sizeof(ia));
            FormatTime(s.maxInclusiveTime,   im, sizeof(im));

            fprintf(stderr, row,
                cs->name, (unsigned long)s.callCount,
                st, sa, sm, it, ia, im);
        }

        fprintf(stderr, "%s", line);
        fprintf(stderr, "  %zu callsite(s), %lu total call(s)\n",
            entries.size(), (unsigned long)totalCalls);
    #if IP_EXPORT_TRACE
        fprintf(stderr, "\n  Trace exported to iptrace.json — view at https://ui.perfetto.dev\n");
    #endif
        fprintf(stderr, "%s\n", line);
    }

    
    void Profiler::ExportTrace() {
        
        // TODO
        //
        //
        // #if IP_EXPORT_TRACE
        //
        //     IP_INFO("Begin Trace");
        //
        //     std::ofstream File("iptrace.json", std::ios::binary | std::ios::trunc);
        //     bool first = true;
        // #endif
        //
        // #if IP_EXPORT_TRACE                                             
        //
        // if (first) { File << "[\n"; first = false; }
        // else       { File << ",\n"; }
        //
        // if (File.is_open()) {
        //     const auto* cs = reinterpret_cast<const CallsiteInfo*>(rec.callsiteInfo);
        //
        //     File << "  {"
        //         << "\"name\": \"" << (cs ? cs->name : "unknown") << "\","
        //         << "\"cat\": \"function\","
        //         << "\"ph\": \"X\","
        //         << "\"ts\": " << rec.startTime / 1000 << ","
        //         << "\"dur\": " << rec.inclusiveTime / 1000 << ","
        //         << "\"pid\": 0,"
        //         << "\"tid\": " << rec.threadID << ",";
        //
        //     File << "\"args\": { \"self\": " << rec.selfTime / 1000 << " }"
        //         << "}";
        // }
        // #endif
        //
        //
        // #if IP_EXPORT_TRACE
        //     File << "\n]\n";
        //     File.flush();
        // #endif
        //

    }

}
