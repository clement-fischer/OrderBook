cmake_minimum_required (VERSION 3.10)
project (CentralLimitOrderBook)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (CMAKE_CXX_STANDARD 17)

find_package(Threads)

add_subdirectory (src)
enable_testing ()
add_subdirectory (test)
