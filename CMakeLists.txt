cmake_minimum_required(VERSION 3.2)
project(v8-demo)

include_directories(/Users/juneleo/v8/v8/include)
include_directories(/Users/juneleo/v8/v8)

link_directories(
        /Users/juneleo/v8/v8/out.gn/x64.release.sample/obj
)
link_libraries(
        v8_monolith
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")

add_executable(HelloWorld ./helloworld.cc)
add_executable(Process ./process.cc)
add_executable(Shell ./shell.cc)
