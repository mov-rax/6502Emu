# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.23

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/movrax/Documents/6502Emu

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/movrax/Documents/6502Emu/build

# Include any dependencies generated for this target.
include CMakeFiles/6502Emu.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/6502Emu.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/6502Emu.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/6502Emu.dir/flags.make

CMakeFiles/6502Emu.dir/main.cpp.o: CMakeFiles/6502Emu.dir/flags.make
CMakeFiles/6502Emu.dir/main.cpp.o: ../main.cpp
CMakeFiles/6502Emu.dir/main.cpp.o: CMakeFiles/6502Emu.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/movrax/Documents/6502Emu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/6502Emu.dir/main.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/6502Emu.dir/main.cpp.o -MF CMakeFiles/6502Emu.dir/main.cpp.o.d -o CMakeFiles/6502Emu.dir/main.cpp.o -c /home/movrax/Documents/6502Emu/main.cpp

CMakeFiles/6502Emu.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/6502Emu.dir/main.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/movrax/Documents/6502Emu/main.cpp > CMakeFiles/6502Emu.dir/main.cpp.i

CMakeFiles/6502Emu.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/6502Emu.dir/main.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/movrax/Documents/6502Emu/main.cpp -o CMakeFiles/6502Emu.dir/main.cpp.s

# Object files for target 6502Emu
6502Emu_OBJECTS = \
"CMakeFiles/6502Emu.dir/main.cpp.o"

# External object files for target 6502Emu
6502Emu_EXTERNAL_OBJECTS =

6502Emu: CMakeFiles/6502Emu.dir/main.cpp.o
6502Emu: CMakeFiles/6502Emu.dir/build.make
6502Emu: 6502Emu_lib/lib6502Emu_lib.a
6502Emu: CMakeFiles/6502Emu.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/movrax/Documents/6502Emu/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable 6502Emu"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/6502Emu.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/6502Emu.dir/build: 6502Emu
.PHONY : CMakeFiles/6502Emu.dir/build

CMakeFiles/6502Emu.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/6502Emu.dir/cmake_clean.cmake
.PHONY : CMakeFiles/6502Emu.dir/clean

CMakeFiles/6502Emu.dir/depend:
	cd /home/movrax/Documents/6502Emu/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/movrax/Documents/6502Emu /home/movrax/Documents/6502Emu /home/movrax/Documents/6502Emu/build /home/movrax/Documents/6502Emu/build /home/movrax/Documents/6502Emu/build/CMakeFiles/6502Emu.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/6502Emu.dir/depend

