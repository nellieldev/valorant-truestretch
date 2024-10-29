cmake_minimum_required(VERSION 3.28)
project(valorant-truestretch)

if (MSVC)
    add_compile_options(/Z7)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG:NONE")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

set(CMAKE_CXX_STANDARD 17)

add_executable(valorant-truestretch WIN32 main.cpp)
