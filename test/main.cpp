#include <thread>


#include "instprof.h"


#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>


static void busy_work_ms(int ms) {

    IP_NAMED_SCOPE("busy_work_ms");
    using namespace std::chrono;
    const auto start = steady_clock::now();
    // Simple CPU spin (mix in a tiny branch to avoid compiler optimizations)
    uint64_t s = 0;
    while (duration_cast<milliseconds>(steady_clock::now() - start).count() < ms) {
        for (int i = 0; i < 10'000; i++)
            s += (i ^ (s + 0x9e3779b97f4));
    }
    (void)s;
}

static void sleep_ms(int ms) {

    IP_NAMED_SCOPE("sleep_ms");
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// --- Workloads ---------------------------------------------------------------

static uint64_t fib(int n) {

    IP_NAMED_SCOPE("fib"); // stress recursion
    if (n <= 1) return n;
    busy_work_ms(0);
    return fib(n - 1) + fib(n - 2);
}

void rec_n2(int n) {

    IP_NAMED_SCOPE("rec_n2");
    if (n <= 0) return;
    for (int i = 0; i < n; ++i)
        busy_work_ms(0);
    rec_n2(n - 1);
}

static void cpu_task(int n) {

    IP_FUNC_SCOPE();
    // Mix recursion + busy spin
    auto f = fib(n);
    //rec_n2(n + 10);
    busy_work_ms(30);
    
    // Some reduction
    volatile long sink = f;
    (void)sink;
}

static void mem_churn(size_t N) {

    IP_FUNC_SCOPE();
    std::vector<int> v;
    v.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        IP_NAMED_SCOPE("push_back");
        v.push_back(static_cast<int>(i * 7 + 3));
    }
    // Shuffle to get cache misses
    std::mt19937_64 rng{123456};
    for (size_t i = 0; i < N; ++i) {
        IP_NAMED_SCOPE("random_access");
        auto idx = static_cast<size_t>(rng() % N);
        v[idx] ^= static_cast<int>(idx);
    }
}

static void io_task(const std::filesystem::path& p, int lines) {

    IP_FUNC_SCOPE();
    std::ofstream out(p, std::ios::binary);
    for (int i = 0; i < lines; ++i) {
        IP_NAMED_SCOPE("write_line");
        out << "line " << i << " : " << std::string(200, 'x') << "\n";
        out.flush();
        sleep_ms(5);
    }
}

static void exc_inner() {

    IP_FUNC_SCOPE();
    // Nested scopes + throw to test unwinding destruction paths
    {
        IP_NAMED_SCOPE("prepare");
        busy_work_ms(10);
    }
    {
        IP_NAMED_SCOPE("throw");
        throw std::runtime_error("boom");
    }
}

static void exc_task() {

    IP_FUNC_SCOPE();
    try {
        exc_inner();
    } catch (const std::exception& e) {
        IP_NAMED_SCOPE("caught");
        (void)e;
        busy_work_ms(5);
    }
}

static void lock_test(std::mutex& mtx, int iters) {

    IP_FUNC_SCOPE();
    for (int i = 0; i < iters; ++i) {
        {
            IP_NAMED_SCOPE("pre_lock_work");
            busy_work_ms(1);
        }
        {
            IP_NAMED_SCOPE("lock_hold");
            std::lock_guard<std::mutex> g(mtx);
            busy_work_ms(2);
        }
    }
}

// --- Main Demo ---------------------------------------------------------------

int main() {

    IP_FUNC_SCOPE();

    {
        cpu_task(6);
    }

    // // Async CPU + memory + exceptions
    // auto f_cpu = std::async(std::launch::async, []{
    //     IP_NAMED_SCOPE("async_cpu");
    //     cpu_task(22);
    // });

    // auto f_mem = std::async(std::launch::async, []{
    //     IP_NAMED_SCOPE("async_mem");
    //     mem_churn(30000);
    // });

    // auto f_exc = std::async(std::launch::async, []{
    //     IP_NAMED_SCOPE("async_exc");
    //     exc_task();
    // });

    // // I/O in parallel
    // std::filesystem::path tmp = std::filesystem::temp_directory_path() / "instprof_demo.txt";
    // auto f_io = std::async(std::launch::async, [tmp]{
    //     IP_NAMED_SCOPE("async_io");
    //     io_task(tmp, 100);
    // });

    // // Some mutex contention across several threads
    // std::mutex mtx;
    // std::vector<std::jthread> lockers;
    // for (int t = 0; t < 4; ++t) {
    //     lockers.emplace_back([&, t]{
    //         IP_NAMED_SCOPE("locker_thread");
    //         lock_test(mtx, 50 + t * 10);
    //     });
    // }

    // // Loop with a per-iteration scope + nested function call
    // for (int i = 0; i < 8; ++i) {
    //     IP_NAMED_SCOPE("tick");
    //     cpu_task(18);
    //     if (i % 3 == 0) sleep_ms(10);
    // }

    // // Lambda with its own zone to test inlined callsites
    // auto lambda = []{
    //     IP_NAMED_SCOPE("lambda_task");
    //     busy_work_ms(15);
    // };
    // lambda();

    // // Join async
    // f_cpu.get();
    // f_mem.get();
    // f_exc.get();
    // f_io.get();

    // std::filesystem::remove(tmp);
}