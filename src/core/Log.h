#pragma once

#include <cstdint>
#include <format>
#include <ostream>
#include <iostream>


namespace instprof {

    /*
        Not thread safe 
        no runtime formatting -> look into std::vformat
        slow -> Look into fmt::memory_buffer to avoid string allocations
        
        This shouldn't be exposed to the user or even exist in distribution binaries.
    */

    class Log {
    public:    
        
        enum class Level : uint8_t { Trace = 0, Info, Warn, Error, Fatal };

        static void Init(std::ostream& sink = std::clog) {
            
            s_Sink = &sink; 
            SetMinLogLevel(Level::Trace);
        }

        static void SetMinLogLevel(Level lvl) { s_MinLevel = lvl; }

        template<class... Args>
        static void Dispatch(Level lvl, std::format_string<Args...> fmt, Args&&... args) {
            
            if (lvl < s_MinLevel) return;
            Write(*s_Sink, lvl, std::format(fmt, std::forward<Args>(args)...));
        }

        // Convenience wrappers -> Macros are better for my use case
        template<class... A> static void Trace (std::format_string<A...> f, A&&... a){ Dispatch(Level::Trace, f, std::forward<A>(a)...); }
        template<class... A> static void Info  (std::format_string<A...> f, A&&... a){ Dispatch(Level::Info , f, std::forward<A>(a)...); }
        template<class... A> static void Warn  (std::format_string<A...> f, A&&... a){ Dispatch(Level::Warn , f, std::forward<A>(a)...); }
        template<class... A> static void Error (std::format_string<A...> f, A&&... a){ Dispatch(Level::Error, f, std::forward<A>(a)...); }
        template<class... A> static void Fatal (std::format_string<A...> f, A&&... a){ Dispatch(Level::Fatal, f, std::forward<A>(a)...); }
        
    private:
        
        static constexpr std::string_view LevelToString(Level lvl) {
            
            switch (lvl) {
                case Level::Trace: return "TRACE";
                case Level::Info:  return "INFO";
                case Level::Warn:  return "WARN";
                case Level::Error: return "ERROR";
                case Level::Fatal: return "FATAL";
            }
            return "?";
        }

        static void Write(std::ostream& os, Level lvl, std::string_view msg) {
            
            os << "[" << LevelToString(lvl) << "]" << ": " << msg << "\n";
            os.flush(); // Syscall
        }
        
    private:

        inline static Level s_MinLevel = Level::Trace;
        inline static std::ostream* s_Sink = &std::clog;
    };
}


#define IP_TRACE(...) ::instprof::Log::Dispatch(::instprof::Log::Level::Trace, __VA_ARGS__)
#define IP_INFO(...)  ::instprof::Log::Dispatch(::instprof::Log::Level::Info, __VA_ARGS__)
#define IP_WARN(...)  ::instprof::Log::Dispatch(::instprof::Log::Level::Warn, __VA_ARGS__)
#define IP_ERROR(...) ::instprof::Log::Dispatch(::instprof::Log::Level::Error, __VA_ARGS__)
#define IP_FATAL(...) ::instprof::Log::Dispatch(::instprof::Log::Level::Fatal, __VA_ARGS__)


