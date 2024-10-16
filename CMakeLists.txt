cmake_minimum_required(VERSION 3.10)
project(teenyKV)

set(CMAKE_CXX_STANDARD 14)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(YAML-CPP REQUIRED)

find_library(LEVELDB_LIBRARY leveldb REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(teenyKV ${SOURCES})
target_include_directories(teenyKV PRIVATE include)
target_link_libraries(teenyKV PRIVATE Boost::system yaml-cpp Boost::thread pthread ${LEVELDB_LIBRARY})
