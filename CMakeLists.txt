cmake_minimum_required(VERSION 3.14)
project(nes_emulator C)

# Set output directories - binaries go to bin/ at project root
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Main emulator library
add_library(emulator_lib
    src/cpu.c
    src/ines.c
    src/bus.c
    src/ppu.c
)

target_include_directories(emulator_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# CPU trace tool
add_executable(cpu_trace src/cpu_trace.c)
target_link_libraries(cpu_trace PRIVATE emulator_lib)

# SDL2 Visual Debugger (optional - only built if SDL2 is found)
find_package(SDL2 QUIET)
if(SDL2_FOUND)
    add_executable(debugger src/debugger.c)
    target_link_libraries(debugger PRIVATE emulator_lib SDL2::SDL2)
    message(STATUS "SDL2 found - building debugger")
else()
    message(STATUS "SDL2 not found - debugger will not be built")
endif()

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

    add_executable(ppu_tests src/ppu_tests.c)
    target_link_libraries(ppu_tests PRIVATE emulator_lib unity)
endif()
