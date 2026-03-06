#include <benchmark/benchmark.h>
#include "core/SPSCQueue.h"
#include "core/Event.h"

#include <thread>
#include <atomic>

// SPSCQueue single-threaded: push then pop
static void BM_SPSCQueue_PushPop(benchmark::State& state) {
    instprof::SPSCQueue<instprof::EventItem, 4096> q;
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    
    for (auto _ : state) {
        q.TryPush(e);
        instprof::EventItem out;
        q.TryPop(out);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SPSCQueue_PushPop);

// SPSCQueue batch operations
static void BM_SPSCQueue_Batch(benchmark::State& state) {
    instprof::SPSCQueue<instprof::EventItem, 4096> q;
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    const size_t batch = state.range(0);
    
    for (auto _ : state) {
        for (size_t i = 0; i < batch; ++i) q.TryPush(e);
        instprof::EventItem out;
        for (size_t i = 0; i < batch; ++i) q.TryPop(out);
    }
    state.SetItemsProcessed(state.iterations() * batch);
}
BENCHMARK(BM_SPSCQueue_Batch)->Arg(64)->Arg(256)->Arg(1024);

// SPSCQueue with producer-consumer contention (1 producer, 1 consumer)
static void BM_SPSCQueue_Contention(benchmark::State& state) {
    static instprof::SPSCQueue<instprof::EventItem, 65536> q;
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    
    if (state.thread_index() == 0) {
        // Producer
        for (auto _ : state) {
            while (!q.TryPush(e)) {}
        }
    } else {
        // Consumer
        instprof::EventItem out;
        for (auto _ : state) {
            while (!q.TryPop(out)) {}
            benchmark::DoNotOptimize(out);
        }
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SPSCQueue_Contention)->Threads(2);

BENCHMARK_MAIN();
