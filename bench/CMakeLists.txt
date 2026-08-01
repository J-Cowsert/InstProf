
find_package(benchmark CONFIG)

if (NOT benchmark_FOUND)

    message(STATUS "Using CMake Version ${CMAKE_VERSION}")
    message(STATUS "Downloading GoogleBenchmark")

    include(FetchContent)

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
    )
    FetchContent_MakeAvailable(googlebenchmark)

endif()

add_executable(workload workload.cpp)
target_link_libraries(workload PRIVATE InstProf)

add_executable(bench_spscqueue bench_spscqueue.cpp)
target_link_libraries(bench_spscqueue PRIVATE InstProf benchmark::benchmark)
