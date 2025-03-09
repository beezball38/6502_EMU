cmake_minimum_required(VERSION 3.11)
project(cpu_emulator VERSION 1.0 LANGUAGES CXX)

# Set output directories first (before defining targets)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Modern C++ standard setup
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create library from source files
add_library(cpu_emulator_lib STATIC
  src/cpu.cpp    # Explicitly list your source files
  )
target_include_directories(cpu_emulator_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  )

# Main executable
add_executable(cpu_emulator src/cpu_emulator.cpp)  # Your main.cpp
target_link_libraries(cpu_emulator PRIVATE cpu_emulator_lib)
