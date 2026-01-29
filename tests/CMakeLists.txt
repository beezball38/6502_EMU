# Create test executable
add_executable(emulator_tests
    cpu_tests.c
)

# Link with main library and Unity
target_link_libraries(emulator_tests
    PRIVATE
        emulator_lib
        unity
)

# Set test executable output directory
set_target_properties(emulator_tests
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Enable CTest
include(CTest)
add_test(NAME emulator_tests COMMAND emulator_tests)
