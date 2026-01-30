cmake_minimum_required(VERSION 3.14)
project(6502_emulator C)

# Set output directories - binaries go to bin/ at project root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Main emulator library
add_library(emulator_lib
    src/cpu.c
    src/ines.c
    src/bus.c
)

target_include_directories(emulator_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Main executable
add_executable(emulator_main src/cpu_emulator.c)
target_link_libraries(emulator_main PRIVATE emulator_lib)

# CPU trace tool
add_executable(cpu_trace src/cpu_trace.c)
target_link_libraries(cpu_trace PRIVATE emulator_lib)

# Testing configuration
option(BUILD_TESTS "Build test suite" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.5.2
    )
    FetchContent_MakeAvailable(unity)

    add_executable(emulator_tests src/cpu_tests.c)
    target_link_libraries(emulator_tests PRIVATE emulator_lib unity)
endif()
