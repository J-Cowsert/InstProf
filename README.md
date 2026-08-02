# InstProf


> [!Warning]
> This repository is undergoing active development. Expect possible modifications to code structure, features, or dependencies.


InstProf is a lightweight C++ profiling library that instruments code execution using RAII-based scope tracking. It records timing information for annotated code regions (called "zones"), aggregates per-callsite statistics, and exports traces for visualization.


Primary Use Cases:

- Performance profiling of single/multi-threaded applications
- Identifying hot paths and bottlenecks
- Understanding recursive call patterns
- Analyzing time distribution across nested function calls
- Exporting execution traces for visualization in external visualization tools

---

### Key Features

| Feature                      | Description                                                                 |
|------------------------------|-----------------------------------------------------------------------------|
| RAII Instrumentation         | Automatic zone entry/exit tracking                    |
| Zero Runtime Overhead When Disabled | IP_ENABLE=0 compiles to no-ops; profiling code is eliminated                |
| Lock-Free Hot Path           | Each thread owns a single-producer/single-consumer queue. Producers never take a lock and never contend with one another |
| Asynchronous Processing      | A dedicated consumer thread pairs events, computes timings, and aggregates statistics off the instrumented thread |
| Nested Zone Support          | Tracks call depth and calculates inclusive vs. self time                   |
| Per-Callsite Aggregation     | Accumulates statistics for each instrumentation point |
| Chrome Trace Export          | Generates iptrace.json for external visualization tools supporting Trace Event Format              |

---

### Quick Example

This is `examples/demo.cpp`, built by default:

```cpp
#include <instprof.h>

#include <thread>

static void do_work(int items) {
    IP_FUNC_SCOPE();
    for (int i = 0; i < items; ++i) {
        IP_NAMED_SCOPE("item");
        volatile long x = 0;                     // stand-in for real work
        for (int j = 0; j < 5000; ++j) x += j;
    }
}

int main() {
    IP_FUNC_SCOPE();

    std::thread t([] { do_work(50); });
    do_work(100);
    t.join();
}
```

Running it prints a statistics report at shutdown:

```
  ──────────────────────────────────────────────────────────────────────────────────────────────────
  instprof — Session Statistics  (sorted by total self time)
  ──────────────────────────────────────────────────────────────────────────────────────────────────
  Name                        Calls   Self Tot   Self Avg   Self Max   Incl Tot   Incl Avg   Incl Max
  ──────────────────────────────────────────────────────────────────────────────────────────────────
  item                          150    469.0 us     3.1 us    14.8 us   469.0 us     3.1 us    14.8 us
  main                            1    111.6 us   111.6 us   111.6 us   425.4 us   425.4 us   425.4 us
  do_work                         2     68.4 us    34.2 us    63.2 us   537.5 us   268.7 us   313.8 us
  ──────────────────────────────────────────────────────────────────────────────────────────────────
  3 callsite(s), 153 total call(s)

  Trace exported to iptrace.json — view at https://ui.perfetto.dev
  ──────────────────────────────────────────────────────────────────────────────────────────────────
```

and writes `iptrace.json` (in the working directory) — drop it into https://ui.perfetto.dev to see the zones on a timeline, one track per thread.

---

### Prerequisites

InstProf requires:

| Requirement   | Minimum Version     |
|---------------|---------------------|
| CMake         | 3.25                |
| C++ Compiler  | C++20 support       |
| Platform      | Linux (primary)     |

Currently, InstProf supports Linux on x86_64 architecture. The build system enforces these requirements through compile-time checks in
src/core/Core.h

---

### Building

```bash
git clone https://github.com/J-Cowsert/InstProf
cd InstProf

cmake --preset release
cmake --build --preset release

./build/release/examples/demo
```

`cmake --list-presets` shows all available configurations. Plain `cmake -S . -B build && cmake --build build` also works and defaults to RelWithDebInfo.

#### Build Options

| CMake Option        | Type    | Default        | Description                                                        |
|---------------------|---------|----------------|--------------------------------------------------------------------|
| IP_ENABLE           | Boolean | ON             | Enable profiler instrumentation. OFF compiles all macros to no-ops |
| IP_EXPORT_TRACE     | Boolean | ON             | Write `iptrace.json` at shutdown. OFF for statistics only          |
| IP_BUILD_BENCHMARKS | Boolean | OFF            | Build the perf workload and Google Benchmark microbenches (uses the system Google Benchmark if installed, fetches it otherwise) |
| CMAKE_BUILD_TYPE    | String  | RelWithDebInfo | Build configuration                                                |

---

### Integrating Into Your Project

Add InstProf as a subdirectory in your project's CMakeLists.txt:

```cmake
add_subdirectory(path/to/InstProf)
target_link_libraries(YourExecutable PRIVATE InstProf)
```

or fetch it directly:

```cmake
include(FetchContent)
FetchContent_Declare(instprof
    GIT_REPOSITORY https://github.com/J-Cowsert/InstProf
    GIT_TAG        master)
FetchContent_MakeAvailable(instprof)

target_link_libraries(YourExecutable PRIVATE InstProf)
```

Linking the target is all that's required — include paths, the C++20 requirement, and configuration defines propagate automatically. When consumed this way, InstProf builds only the library: examples, benchmarks, and development tooling stay out of your build, and none of your project's global settings are touched.

---

### Instrumenting Your Code

Include the single public header in your source files:

```cpp
#include <instprof.h>
```

Two macros instrument code:

| Macro                     | Purpose                    | Zone Name                           |
|---------------------------|----------------------------|-------------------------------------|
| IP_FUNC_SCOPE()           | Profile entire function    | Uses `__func__` (function name)     |
| IP_NAMED_SCOPE("name")    | Profile a code block       | Uses the provided string literal    |

Both create an RAII object: the zone opens where the macro appears and closes at the end of the enclosing scope. Zones nest freely — the profiler tracks depth and computes inclusive time (total) and self time (excluding children) for each callsite.

```cpp
void my_function() {
    IP_FUNC_SCOPE();                      // zone: "my_function"

    for (int i = 0; i < 1000; ++i) {
        IP_NAMED_SCOPE("loop_iteration"); // zone: "loop_iteration", 1000 calls
        // Work here
    }
}
```

---

### Development

Repository layout:

```
src/        the InstProf library (the only thing consumers build)
examples/   demo.cpp — minimal instrumented program, built by default
bench/      perf workload + Google Benchmark microbenches (IP_BUILD_BENCHMARKS=ON)
scripts/    bench.py (benchmark runner), overhead_compare.sh (IP_ENABLE on/off comparison)
```

Configure presets: `debug`, `release`, `bench` (benchmarks + workload, RelWithDebInfo), `asan` (AddressSanitizer + UBSan), `tsan` (ThreadSanitizer).

```bash
cmake --preset tsan && cmake --build --preset tsan
./build/tsan/bench/workload        # race-check the queue under load
```

The library compiles as strict C++20 with `-Wall -Wextra` (no GNU extensions). A `.clang-tidy` configuration is provided and picked up automatically by clangd.

---

### Architectural Overview

InstProf follows a producer-consumer architecture. Application threads produce profiling events through RAII instrumentation, and a dedicated worker thread consumes and processes them asynchronously.

**Producers.** Entering a zone emits a `ZoneBegin` event; leaving it emits a `ZoneEnd`. Each thread pushes into its own single-producer/single-consumer ring buffer, reached through a cached `thread_local` pointer, so producers never take a lock and never contend with each other. Threads register themselves on first use.

**Callsites.** Each instrumentation macro creates a static `CallsiteInfo` (name, function, file, line) whose address is registered into a dedicated linker section at link time. Events carry the callsite pointer rather than a string, so no string handling happens on the hot path.

**Consumer.** A single worker thread drains every queue in batches, maintains a per-thread stack of open zones to pair begins with ends, and computes inclusive and self time. Results are aggregated per callsite and retained as zone records for the trace export.

**Shutdown.** The worker is stopped and joined, the trace is written, and the statistics report is printed to stderr.

---

### Current Limitations

- **Consumer throughput is the scaling limit.** One consumer thread services all producers, so per-zone overhead grows as producer thread count rises. When a queue fills, the producing thread spins until space is available.
- **Threads are never deregistered.** Thread entries are retained for the life of the process. This keeps the consumer's queue snapshot safe without synchronization, but means memory grows with the number of threads a process has ever created.
- **Zone records are held in memory until shutdown**, so peak memory scales with total zone count.
- **The trace is written in Chrome Trace Event JSON.** The format remains widely supported, though it is associated with the legacy `chrome://tracing` tool; Perfetto suggests its native protobuf format is worth considering for new tooling.
