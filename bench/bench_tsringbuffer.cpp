#include <benchmark/benchmark.h>
#include "core/TSRingBuffer.h"
#include "core/Event.h"

#include <thread>
#include <atomic>

// TSRingBuffer single-threaded: push then pop
static void BM_TSRingBuffer_PushPop(benchmark::State& state) {
    instprof::TSRingBuffer<instprof::EventItem> q{4096};
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    
    for (auto _ : state) {
        q.Push(e);
        instprof::EventItem out = q.Pop();
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSRingBuffer_PushPop);

// TSRingBuffer batch operations
static void BM_TSRingBuffer_Batch(benchmark::State& state) {
    instprof::TSRingBuffer<instprof::EventItem> q{4096};
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    const size_t batch = state.range(0);
    
    for (auto _ : state) {
        for (size_t i = 0; i < batch; ++i) q.Push(e);
        instprof::EventItem out;
        for (size_t i = 0; i < batch; ++i) out = q.Pop();
    }
    state.SetItemsProcessed(state.iterations() * batch);
}
BENCHMARK(BM_TSRingBuffer_Batch)->Arg(64)->Arg(256)->Arg(1024);

// TSRingBuffer with multithreaded contention (all threads push and pop)
static void BM_TSRingBuffer_Contention(benchmark::State& state) {
    static instprof::TSRingBuffer<instprof::EventItem> q{65536};
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    
    for (auto _ : state) {
        q.Push(e);
        instprof::EventItem out;
        q.TryPop(out);
        benchmark::DoNotOptimize(out);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSRingBuffer_Contention)->ThreadRange(1, 8);

BENCHMARK_MAIN();
