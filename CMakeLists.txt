cmake_minimum_required(VERSION 3.15)
project(process_monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(process_monitor 
    src/main.cpp
    src/process_monitor.cpp)

target_include_directories(process_monitor PRIVATE include)
target_link_libraries(process_monitor pthread)
