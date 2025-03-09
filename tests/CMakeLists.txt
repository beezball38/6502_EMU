# Create test executable
add_executable(emulator_tests
    cpu_tests.cpp
)

# Link with main library and Catch2
target_link_libraries(emulator_tests
    PRIVATE
        emulator_lib
        Catch2::Catch2WithMain
)

# Set test executable output directory
set_target_properties(emulator_tests
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Enable test discovery
include(CTest)
include(Catch)
catch_discover_tests(emulator_tests)
