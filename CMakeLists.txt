cmake_minimum_required(VERSION 3.19)
project(6502Emu)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES main.cpp)
add_executable(6502Emu ${SOURCE_FILES})


add_subdirectory(fmt)

include_directories(6502Emu_lib)
add_subdirectory(6502Emu_lib)

target_link_libraries(6502Emu fmt::fmt 6502Emu_lib)

add_subdirectory(Catch_tests)