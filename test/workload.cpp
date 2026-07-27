
#include "instprof.h"

#include <barrier>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

// Toggle scenarios on/off here
constexpr bool RUN_FLAT       = true;
constexpr bool RUN_NESTED     = true;
constexpr bool RUN_RECURSION  = true;
constexpr bool RUN_MULTITHREAD = true;

static inline void DoNotOptimize(uint64_t& val) {
    asm volatile("" : "+r"(val) :: "memory");
}

static inline uint64_t Mix(uint64_t s, uint64_t v) {
    s ^= v + 0x9e3779b97f4a7c15ULL + (s << 6) + (s >> 2);
    return s;
}

static double WallMs(std::chrono::steady_clock::time_point t0,
                     std::chrono::steady_clock::time_point t1) {
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// ---------------------------------------------------------------------------

static double FlatScopes() {
    constexpr int N = 1'000'000;
    uint64_t s = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        IP_NAMED_SCOPE("flat");
        s = Mix(s, (uint64_t)i);
        DoNotOptimize(s);
    }
    auto t1 = std::chrono::steady_clock::now();

    DoNotOptimize(s);
    return WallMs(t0, t1);
}

static double NestedScopes() {
    constexpr int N = 125'000;
    uint64_t s = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        IP_NAMED_SCOPE("d1"); s = Mix(s, (uint64_t)i + 1); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d2"); s = Mix(s, (uint64_t)i + 2); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d3"); s = Mix(s, (uint64_t)i + 3); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d4"); s = Mix(s, (uint64_t)i + 4); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d5"); s = Mix(s, (uint64_t)i + 5); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d6"); s = Mix(s, (uint64_t)i + 6); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d7"); s = Mix(s, (uint64_t)i + 7); DoNotOptimize(s);
        { IP_NAMED_SCOPE("d8"); s = Mix(s, (uint64_t)i + 8); DoNotOptimize(s);
        }}}}}}}
    }
    auto t1 = std::chrono::steady_clock::now();

    DoNotOptimize(s);
    return WallMs(t0, t1);
}

static uint64_t RecurseImpl(int depth, uint64_t s) {
    IP_NAMED_SCOPE("recurse");
    s = Mix(s, (uint64_t)depth);
    DoNotOptimize(s);
    if (depth == 0) return s;
    return RecurseImpl(depth - 1, s);
}

static double Recursion() {
    constexpr int N = 125'000;
    constexpr int D = 7; // depth 0..7 = 8 zone entries per call
    uint64_t s = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        s ^= RecurseImpl(D, s);
        DoNotOptimize(s);
    }
    auto t1 = std::chrono::steady_clock::now();

    DoNotOptimize(s);
    return WallMs(t0, t1);
}

static double MultiThread() {
    constexpr int T = 4;
    constexpr int N = 250'000;

    std::barrier bar(T + 1);
    std::vector<std::thread> threads;
    threads.reserve(T);

    // spawn threads before timing
    for (int t = 0; t < T; ++t) {
        threads.emplace_back([&, t] {
            bar.arrive_and_wait(); // wait for go signal
            uint64_t s = (uint64_t)(t + 1);
            for (int i = 0; i < N; ++i) {
                IP_NAMED_SCOPE("mt");
                s = Mix(s, (uint64_t)i);
                DoNotOptimize(s);
            }
            DoNotOptimize(s);
        });
    }

    // start timing after threads are ready
    auto t0 = std::chrono::steady_clock::now();
    bar.arrive_and_wait();

    for (auto& th : threads) th.join();
    auto t1 = std::chrono::steady_clock::now();

    return WallMs(t0, t1);
}

// ---------------------------------------------------------------------------

int main() {
    // warmup
    {
        uint64_t s = 0;
        for (int i = 0; i < 10'000; ++i) {
            IP_NAMED_SCOPE("warmup");
            s = Mix(s, (uint64_t)i);
            DoNotOptimize(s);
        }
        DoNotOptimize(s);
    }

    double total = 0;
    fprintf(stdout, "%-18s %10s %12s\n", "scenario", "zones", "wall_ms");

    if constexpr (RUN_FLAT) {
        double ms = FlatScopes();
        fprintf(stdout, "%-18s %10d %12.3f\n", "flat_scopes", 1000000, ms);
        total += ms;
    }
    if constexpr (RUN_NESTED) {
        double ms = NestedScopes();
        fprintf(stdout, "%-18s %10d %12.3f\n", "nested_scopes", 1000000, ms);
        total += ms;
    }
    if constexpr (RUN_RECURSION) {
        double ms = Recursion();
        fprintf(stdout, "%-18s %10d %12.3f\n", "recursion", 1000000, ms);
        total += ms;
    }
    if constexpr (RUN_MULTITHREAD) {
        double ms = MultiThread();
        fprintf(stdout, "%-18s %10d %12.3f\n", "multi_thread", 1000000, ms);
        total += ms;
    }

    fprintf(stdout, "%-18s %10s %12.3f\n", "total", "-", total);
}
