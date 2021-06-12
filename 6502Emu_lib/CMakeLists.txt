project(6502Emu_lib)

set(HEADER_FILES
        cpu.hpp
        instruction.hpp
        mem.hpp
        types.h)

set(SOURCE_FILES
        cpu.cpp
        instruction.cpp
        mem.cpp)

add_library(6502Emu_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})