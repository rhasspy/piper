# C++ Tests CMake configuration

# Test sources
set(TEST_SOURCES
    test_phonemize.cpp
    test_piper_core.cpp
)

# Add dictionary manager test for Unix platforms
if(NOT WIN32 AND NOT MSVC)
    list(APPEND TEST_SOURCES test_dictionary_manager.cpp)
endif()

# Create test executables
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})

    target_link_libraries(${test_name}
        gtest
        gtest_main
    )
    
    # Link additional dependencies for dictionary manager test
    if(${test_name} STREQUAL "test_dictionary_manager" AND NOT WIN32 AND NOT MSVC)
        target_sources(${test_name} PRIVATE
            ../openjtalk_dictionary_manager.c
        )
        target_compile_definitions(${test_name} PRIVATE
            OPENJTALK_DIC_PATH="../share/open_jtalk/dic"
        )
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()