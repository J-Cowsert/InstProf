
cmake_minimum_required(VERSION 3.25)
project(instprof LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PROF_ENABLE "Enable profiler instrumentation" ON)

file(GLOB_RECURSE SRC_FILES
     CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(InstProf STATIC ${SRC_FILES})

target_include_directories(InstProf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

if(PROF_ENABLE)
    target_compile_definitions(InstProf PUBLIC PROF_ENABLE)  
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(InstProf PUBLIC IP_PLATFORM_LINUX)
endif()

