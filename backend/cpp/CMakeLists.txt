cmake_minimum_required(VERSION 3.16)
project(metis_backtester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(metis_backtester
  src/main.cpp
  src/backtest.cpp
)

target_include_directories(metis_backtester PRIVATE include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(metis_backtester PRIVATE -static-libstdc++ -static-libgcc)
endif()
