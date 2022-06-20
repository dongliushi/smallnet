project(net)
cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_COMPILER /opt/rh/devtoolset-9/root/bin/gcc)
set(CMAKE_CXX_COMPILER /opt/rh/devtoolset-9/root/bin/g++)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src bin)
