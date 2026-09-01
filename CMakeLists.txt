cmake_minimum_required(VERSION 4.3)
project(hlvc)

set(CMAKE_CXX_STANDARD 17)

add_library(hlvc HLVCDecoder.cpp HLVCDecoder.hpp)

add_executable(hlvc_example example_hlvc.cpp)
target_link_libraries(hlvc_example hlvc)