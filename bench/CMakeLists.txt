include(FetchContent)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.1
)
FetchContent_MakeAvailable(googlebenchmark)

file(GLOB BENCH_SOURCES CONFIGURE_DEPENDS bench_*.cpp)

foreach(src ${BENCH_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE InstProf benchmark::benchmark)
endforeach()
