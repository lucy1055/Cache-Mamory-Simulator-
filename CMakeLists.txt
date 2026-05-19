cmake_minimum_required(VERSION 3.16)
project(CacheMemorySimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cache_sim
    src/main.cpp
    src/cache.cpp
)

target_include_directories(cache_sim PRIVATE include)
