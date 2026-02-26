#include <benchmark/benchmark.h>
#include "core/TSRingBuffer.h"
#include "core/Event.h"

#include <cstddef>
#include <queue>

static void BM_stdqueue(benchmark::State& state) {
    
    std::queue<instprof::EventItem> q;
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    const size_t N = state.range(0);
    
    for (auto _ : state) {
        for (size_t i = 0; i < N; ++i) q.push(e);
        for (size_t i = 0; i < N; ++i) {
            benchmark::DoNotOptimize(q.front());
            q.pop();
        }
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_stdqueue)->Range(64, 4096);

static void BM_TSRingBuffer(benchmark::State& state) {
    
    instprof::TSRingBuffer<instprof::EventItem> q {4096};
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;
    const size_t N = state.range(0);
    
    for (auto _ : state) {
        for (size_t i = 0; i < N; ++i) q.Push(e);
        for (size_t i = 0; i < N; ++i) {
            benchmark::DoNotOptimize(q.Pop());
        }
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_TSRingBuffer)->Range(64, 4096);

static void BM_TSRingBuffer_Produce(benchmark::State& state) {
    static instprof::TSRingBuffer<instprof::EventItem> q{65536};
    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;

    for (auto _ : state) {
        q.Push(e);
        q.Pop();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TSRingBuffer_Produce)->ThreadRange(1, 8);

static void BM_TSRingBuffer_ProducerConsumer(benchmark::State& state) {
    static instprof::TSRingBuffer<instprof::EventItem> q{65536};
    static std::atomic<bool> running{true};

    std::thread consumer;
    if (state.thread_index() == 0) {
        running.store(true);
        consumer = std::thread([&] {
            instprof::EventItem out;
            while (running.load(std::memory_order_relaxed)) {
                q.TryPop(out);
            }
            // drain remaining
            while (q.TryPop(out)) {}
        });
    }

    instprof::EventItem e;
    e.tag.type = instprof::EventType::ZoneBegin;

    for (auto _ : state) {
        q.Push(e);
    }

    state.SetItemsProcessed(state.iterations());

    if (state.thread_index() == 0) {
        running.store(false);
        consumer.join();
    }
}
BENCHMARK(BM_TSRingBuffer_ProducerConsumer)->ThreadRange(1, 8)->UseRealTime();
BENCHMARK(BM_TSRingBuffer_ProducerConsumer)->ThreadRange(2, 8);

BENCHMARK_MAIN();
