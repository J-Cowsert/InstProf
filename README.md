# InstProf


> [!Warning]
> This repository is undergoing active development. Expect possible modifications to code structure, features, or dependencies.


InstProf is a lightweight C++ profiling library that instruments code execution using RAII-based scope tracking. It records timing information for annotated code regions (called "zones") and aggregates statistics. 


Primary Use Cases:

- Performance profiling of multi-threaded applications
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

```cpp
#include "instprof.h"

void process_data(int n) {
    IP_FUNC_SCOPE();  // Automatically named "process_data"
    
    for (int i = 0; i < n; ++i) {
        IP_NAMED_SCOPE("iteration");  // Custom name
        // ... work here ...
    }
}

int main() {
    IP_FUNC_SCOPE();
    process_data(100);
    // Profiling data automatically collected and exported on shutdown
}
```

This generates:

- Per-thread timing data for each zone
- Aggregate statistics per callsite (e.g., "iteration" called 100 times)
- Nested timing relationships (main → process_data → iteration)
- JSON trace file for external visualization

---

### Prerequisites

InstProf requires: 

| Requirement   | Minimum Version     |
|---------------|---------------------|
| CMake         | 3.25                |
| C++ Compiler  | C++23 support       |
| Platform      | Linux (primary)     |

Currently, InstProf supports Linux on x86_64 architecture. The build system enforces these requirements through compile-time checks in
src/core/Core.h

---

### Building

```bash
git clone https://github.com/J-Cowser/InstProf
cd InstProf

mkdir build
cd build

cmake ..       # configure the project
cmake --build .   # compile
```

#### Build Options Reference

| CMake Option                  | Type    | Default        | Description                                     |
|-------------------------------|---------|----------------|-------------------------------------------------|
| IP_ENABLE                     | Boolean | ON             | Enable profiler instrumentation                 |
| IP_BUILD_BENCHMARKS           | Boolean | OFF            | Build benchmarks (fetches Google Benchmark)     |
| CMAKE_BUILD_TYPE              | String  | RelWithDebInfo | Build configuration                             |
| CMAKE_EXPORT_COMPILE_COMMANDS | Boolean | ON             | Generate compile_commands.json                  |

Configure presets are also provided:

```bash
cmake --preset release      # or: debug, bench
cmake --build --preset release
```

One additional knob is a compile definition rather than a CMake option:

| Definition       | Default | Description                                                      |
|------------------|---------|------------------------------------------------------------------|
| IP_EXPORT_TRACE  | 1       | Write `iptrace.json` at shutdown. Set to 0 for statistics only    |

The IP_ENABLE option defined in CMakeLists.txt controls whether profiling code is compiled into the library. When disabled, instrumentation macros expand to nothing

---

### Integrating Into Your Project

##### CMake Integration

Add InstProf as a subdirectory in your project's CMakeLists.txt:
```cmake
# Add InstProf library
add_subdirectory(path/to/InstProf)

# Link against your executable
target_link_libraries(YourExecutable PRIVATE InstProf)
```
---

### Instrumenting Your Code

To use InstProf, include the single public header in your source files:

```cpp
#include "instprof.h"
```
In this header, InstProf provides two primary macros for instrumenting code:

| Macro                     | Purpose                    | Scope Name                          |
|---------------------------|----------------------------|-------------------------------------|
| IP_FUNC_SCOPE()           | Profile entire function    | Uses `__func__` (function name)     |
| IP_NAMED_SCOPE("name")    | Profile code block         | Uses provided string literal

### Usage Patterns

#### Function-Level Profiling

Add IP_FUNC_SCOPE() at the beginning of any function to profile it. The macro uses the compiler-provided __func__ to automatically name the profiling zone:

```cpp
void my_function() {
    IP_FUNC_SCOPE();
    // Your function body here
}
```

#### Named Scope Instrumentation

Use IP_NAMED_SCOPE(name) to profile specific code blocks with custom names:

```cpp
void my_function() {
    IP_FUNC_SCOPE();

    for (int i = 0; i < 1000; ++i) {
        IP_NAMED_SCOPE("loop_iteration");
        // Work here
    }
}
```

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
- **The Chrome Trace Event format is a legacy format.** It remains widely supported, but Perfetto now recommends its native protobuf format for new instrumentation libraries.
