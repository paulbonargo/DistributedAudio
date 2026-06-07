cmake_minimum_required(VERSION 3.15)
project(DistributedAudio VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)
add_subdirectory(plugin)
# add_subdirectory(receiver)