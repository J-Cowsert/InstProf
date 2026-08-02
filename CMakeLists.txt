
cmake_minimum_required(VERSION 3.25)
project(instprof LANGUAGES CXX)


if (PROJECT_IS_TOP_LEVEL)

    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "No build type specified, defaulting to RelWithDebInfo")
        set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    endif()

    set(IP_SANITIZE "" CACHE STRING "Sanitizers: 'address,undefined' or 'thread' (dev builds only)")
    if(IP_SANITIZE)
        add_compile_options(-fsanitize=${IP_SANITIZE} -fno-omit-frame-pointer)
        add_link_options(-fsanitize=${IP_SANITIZE})
    endif()

endif()

set(CMAKE_CXX_EXTENSIONS OFF)

# options
option(IP_ENABLE "Enable profiler instrumentation" ON)
option(IP_EXPORT_TRACE "Write iptrace.json at shutdown" ON)
option(IP_BUILD_BENCHMARKS "Build benchmarks (Requires https://github.com/google/benchmark.git)" OFF)

file(GLOB_RECURSE SRC_FILES
     CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(InstProf STATIC ${SRC_FILES})

if (PROJECT_IS_TOP_LEVEL) 
    target_compile_options(InstProf PRIVATE -Wall -Wextra)
endif()

target_compile_features(InstProf PUBLIC cxx_std_20)
target_include_directories(InstProf PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_compile_definitions(InstProf
    PUBLIC
        IP_ENABLE=$<BOOL:${IP_ENABLE}>
        $<$<CONFIG:Debug>:IP_DEBUG>
        $<$<PLATFORM_ID:Linux>:IP_PLATFORM_LINUX>
    PRIVATE
        IP_EXPORT_TRACE=$<BOOL:${IP_EXPORT_TRACE}>)


if(IP_BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()


