cmake_minimum_required(VERSION 3.16)
project(lockfree_threadpool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)
option(ENABLE_ASAN "Build with AddressSanitizer + UBSan" OFF)

if(ENABLE_TSAN)
  add_compile_options(-fsanitize=thread -g -O1)
  add_link_options(-fsanitize=thread)
endif()
if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address,undefined -g -O1)
  add_link_options(-fsanitize=address,undefined)
endif()

find_package(Threads REQUIRED)

add_library(tp INTERFACE)
target_include_directories(tp INTERFACE include)
target_compile_options(tp INTERFACE -Wall -Wextra -Wpedantic)
target_link_libraries(tp INTERFACE Threads::Threads)

enable_testing()
add_subdirectory(tests)
add_subdirectory(benchmarks)
