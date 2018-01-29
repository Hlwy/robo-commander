# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hunter/devel/robo-dev

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hunter/devel/robo-dev/build

# Include any dependencies generated for this target.
include src/comms/CMakeFiles/udp.dir/depend.make

# Include the progress variables for this target.
include src/comms/CMakeFiles/udp.dir/progress.make

# Include the compile flags for this target's objects.
include src/comms/CMakeFiles/udp.dir/flags.make

src/comms/CMakeFiles/udp.dir/udp.cpp.o: src/comms/CMakeFiles/udp.dir/flags.make
src/comms/CMakeFiles/udp.dir/udp.cpp.o: ../src/comms/udp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hunter/devel/robo-dev/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/comms/CMakeFiles/udp.dir/udp.cpp.o"
	cd /home/hunter/devel/robo-dev/build/src/comms && /usr/bin/c++   $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/udp.dir/udp.cpp.o -c /home/hunter/devel/robo-dev/src/comms/udp.cpp

src/comms/CMakeFiles/udp.dir/udp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/udp.dir/udp.cpp.i"
	cd /home/hunter/devel/robo-dev/build/src/comms && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hunter/devel/robo-dev/src/comms/udp.cpp > CMakeFiles/udp.dir/udp.cpp.i

src/comms/CMakeFiles/udp.dir/udp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/udp.dir/udp.cpp.s"
	cd /home/hunter/devel/robo-dev/build/src/comms && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hunter/devel/robo-dev/src/comms/udp.cpp -o CMakeFiles/udp.dir/udp.cpp.s

src/comms/CMakeFiles/udp.dir/udp.cpp.o.requires:

.PHONY : src/comms/CMakeFiles/udp.dir/udp.cpp.o.requires

src/comms/CMakeFiles/udp.dir/udp.cpp.o.provides: src/comms/CMakeFiles/udp.dir/udp.cpp.o.requires
	$(MAKE) -f src/comms/CMakeFiles/udp.dir/build.make src/comms/CMakeFiles/udp.dir/udp.cpp.o.provides.build
.PHONY : src/comms/CMakeFiles/udp.dir/udp.cpp.o.provides

src/comms/CMakeFiles/udp.dir/udp.cpp.o.provides.build: src/comms/CMakeFiles/udp.dir/udp.cpp.o


# Object files for target udp
udp_OBJECTS = \
"CMakeFiles/udp.dir/udp.cpp.o"

# External object files for target udp
udp_EXTERNAL_OBJECTS =

../lib/libudp.so: src/comms/CMakeFiles/udp.dir/udp.cpp.o
../lib/libudp.so: src/comms/CMakeFiles/udp.dir/build.make
../lib/libudp.so: ../lib/libutils.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libboost_system.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libboost_thread.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libboost_chrono.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libboost_date_time.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libboost_atomic.so
../lib/libudp.so: /usr/lib/x86_64-linux-gnu/libpthread.so
../lib/libudp.so: /usr/local/lib/libpigpiod_if2.so
../lib/libudp.so: /usr/lib/libarmadillo.so
../lib/libudp.so: /usr/lib/liblapack.so
../lib/libudp.so: /usr/lib/libblas.so
../lib/libudp.so: /usr/lib/libblas.so
../lib/libudp.so: src/comms/CMakeFiles/udp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hunter/devel/robo-dev/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library ../../../lib/libudp.so"
	cd /home/hunter/devel/robo-dev/build/src/comms && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/udp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/comms/CMakeFiles/udp.dir/build: ../lib/libudp.so

.PHONY : src/comms/CMakeFiles/udp.dir/build

src/comms/CMakeFiles/udp.dir/requires: src/comms/CMakeFiles/udp.dir/udp.cpp.o.requires

.PHONY : src/comms/CMakeFiles/udp.dir/requires

src/comms/CMakeFiles/udp.dir/clean:
	cd /home/hunter/devel/robo-dev/build/src/comms && $(CMAKE_COMMAND) -P CMakeFiles/udp.dir/cmake_clean.cmake
.PHONY : src/comms/CMakeFiles/udp.dir/clean

src/comms/CMakeFiles/udp.dir/depend:
	cd /home/hunter/devel/robo-dev/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hunter/devel/robo-dev /home/hunter/devel/robo-dev/src/comms /home/hunter/devel/robo-dev/build /home/hunter/devel/robo-dev/build/src/comms /home/hunter/devel/robo-dev/build/src/comms/CMakeFiles/udp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/comms/CMakeFiles/udp.dir/depend

