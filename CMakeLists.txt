cmake_minimum_required(VERSION 3.10)
project(LowLatencyPipeline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(processing_engine src/main.cpp)

target_link_libraries(processing_engine zmq pthread)
