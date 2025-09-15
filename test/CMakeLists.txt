cmake_minimum_required(VERSION 3.25)

project(test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_subdirectory(../ ip)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE InstProf)
