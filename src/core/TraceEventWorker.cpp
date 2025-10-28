#include "TraceEventWorker.h"
#include "Callsite.h"
#include <cstdint>


namespace instprof {

    namespace Util {

        static void JsonEscape(std::string_view sv, std::string& out) {
            out.clear(); out.reserve(sv.size());
            for (unsigned char c : sv) {
                switch (c) {
                    case '\"': out += "\\\""; break; 
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break; 
                    case '\r': out += "\\r"; break; 
                    case '\t': out += "\\t"; break;
                    default: out.push_back(char(c)); 
                }
            }
        }

        //static int64_t NanoToMicroSeconds(int64_t t) { return 0; }
    
    }

    bool TraceEventWorker::Start() {

        if (m_Running.load()) return false;
        m_File = std::fopen(m_Path.c_str(), "wb");
        if (!m_File) return false;
        std::fputs("[\n", m_File);
        m_First = true;
        m_Stop.store(false);
        m_Running.store(true);
        m_Thread = std::thread([this]{ Run(); });
        return true;
    }


    void TraceEventWorker::Stop() {

        if (!m_Running.load()) return;
        m_Stop.store(true);
        if (m_Thread.joinable()) m_Thread.join();
        if (m_File) { std::fputs("\n]\n", m_File); std::fflush(m_File); std::fclose(m_File); m_File = nullptr; }
        m_Running.store(false);
    }

    void TraceEventWorker::Run() {

        EventItem ev; 
        std::string jname, jfunc, jfile;
        uint64_t flushCtr = 0;

        for (;;) {
            if (m_Q.TryPop(ev)) {

                switch (ev.tag.type) {

                    case EventType::ZoneBegin: {

                        auto* cs = reinterpret_cast<const CallsiteInfo*>(ev.zoneBegin.callsiteInfo);
                        Util::JsonEscape(cs ? cs->name     : "<null>", jname);
                        Util::JsonEscape(cs ? cs->function : "<null>", jfunc);
                        Util::JsonEscape(cs ? cs->file     : "<null>", jfile);
                        Comma();
                        std::fprintf(m_File,
                            R"({"name":"%s","cat":"zone","ph":"B","ts":%lld,"pid":0,"tid":%u,"args":{"func":"%s","file":"%s","line":%u}})",
                            jname.c_str(),
                            (long long)(ev.zoneBegin.time + 500)/1000,
                            ev.zoneBegin.threadID,
                            jfunc.c_str(), jfile.c_str(),
                            cs ? cs->line : 0u);
                        break;
                    }
                    case EventType::ZoneEnd: {

                        Comma();
                        std::fprintf(m_File,
                            R"({"cat":"zone","ph":"E","ts":%lld,"pid":0,"tid":%u})",
                            (long long)(ev.zoneEnd.time + 500) / 1000,
                            ev.zoneEnd.threadID);
                        break;
                    }

                    default: break;
                }

                if ((++flushCtr & 1023u) == 0) std::fflush(m_File); // TODO: Currently flush ctr is coupled to Q capacity

            } else {

                if (m_Stop.load()) break; 
                std::this_thread::yield();
            }
        }
        std::fflush(m_File);
    }




}