
cmake_minimum_required(VERSION 3.25)
project(instprof LANGUAGES CXX VERSION 1.0)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified, defaulting to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(IP_ENABLE "Enable profiler instrumentation" ON)

file(GLOB_RECURSE SRC_FILES
     CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(InstProf STATIC ${SRC_FILES})

target_include_directories(InstProf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

if(IP_ENABLE)
    target_compile_definitions(InstProf PUBLIC IP_ENABLE=1)
else()
    target_compile_definitions(InstProf PUBLIC IP_ENABLE=0)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(InstProf PUBLIC IP_PLATFORM_LINUX)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(InstProf PUBLIC IP_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(InstProf PUBLIC IP_RELEASE)
endif()

option(IP_BUILD_BENCHMARKS "Build benchmarks (fetches Google Benchmark)" OFF)

if(IP_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()


