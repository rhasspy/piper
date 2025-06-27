# C++ Tests CMake configuration

# Find Google Test
find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    test_phonemize.cpp
    test_piper_core.cpp
)

# Create test executables
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})

    target_link_libraries(${test_name}
        GTest::GTest
        GTest::Main
    )

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()