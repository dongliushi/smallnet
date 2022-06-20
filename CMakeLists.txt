project(net)
cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_COMPILER /opt/rh/devtoolset-9/root/bin/gcc)
set(CMAKE_CXX_COMPILER /opt/rh/devtoolset-9/root/bin/g++)

include_directories(${PROJECT_SOURCE_DIR}/include)
find_package(Threads)

add_subdirectory(src)

add_executable(test test.cc)

target_link_libraries(test PUBLIC FXH_NET)
target_link_libraries(test PUBLIC ${CMAKE_THREAD_LIBS_INIT})
