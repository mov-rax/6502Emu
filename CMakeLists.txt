cmake_minimum_required(VERSION 3.19)
project(6502Emu)

set(CMAKE_CXX_STANDARD 20)

add_executable(6502Emu main.cpp instruction.hpp instruction.cpp cpu.hpp cpu.cpp types.h)