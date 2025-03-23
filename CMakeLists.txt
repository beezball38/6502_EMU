cmake_minimum_required(VERSION 3.14)
project(6502_emulator)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Main emulator library
add_library(emulator_lib
    src/cpu.cpp
)

# Main executable
add_executable(emulator_main
    src/cpu_emulator.cpp
)
target_link_libraries(emulator_main PRIVATE emulator_lib)

# Include directories
target_include_directories(emulator_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Testing configuration
option(BUILD_TESTS "Build test suite" ON)

if(BUILD_TESTS)# Try to find Catch2 in the system first
    find_package(Catch2 QUIET)
    if (NOT Catch2_FOUND)
        include(FetchContent)
        set(FETCHCONTENT_FULLY_DISCONNECTED ON)
        FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
        )
        FetchContent_MakeAvailable(Catch2)
    endif()

    add_subdirectory(tests)
endif()
