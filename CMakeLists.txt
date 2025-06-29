cmake_minimum_required(VERSION 3.15)

# Set runtime library BEFORE project() to ensure it's applied globally
if(MSVC)
  # Force all projects to use static runtime library
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime Library" FORCE)
  # Ensure new CMake policy for MSVC runtime library selection
  cmake_policy(SET CMP0091 NEW)
endif()

project(piper C CXX)

file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" piper_version)
string(STRIP "${piper_version}" piper_version)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Force compiler to use UTF-8 for IPA constants
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  
  # Additional Windows-specific flags
  add_compile_options(/EHsc)  # Enable C++ exceptions
  add_compile_options(/W3)    # Warning level 3
  
  # Define Windows version macros
  add_compile_definitions(WIN32_LEAN_AND_MEAN)
  add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7 and later
elseif(APPLE)
  # macOS flags
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
else()
  # Linux flags
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
endif()

add_executable(piper src/cpp/main.cpp src/cpp/piper.cpp)
add_executable(test_piper src/cpp/test.cpp src/cpp/piper.cpp)


# Testing support
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
  enable_testing()
  
  # Use FetchContent for Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  
  add_subdirectory(src/cpp/tests)
endif()

# Add OpenJTalk support on Unix platforms (not Windows)
if(NOT WIN32 AND NOT MSVC)
  target_sources(piper PRIVATE
    src/cpp/openjtalk_phonemize.cpp
    src/cpp/openjtalk_wrapper.c
    src/cpp/openjtalk_dictionary_manager.c
  )
  target_sources(test_piper PRIVATE
    src/cpp/openjtalk_phonemize.cpp
    src/cpp/openjtalk_wrapper.c
    src/cpp/openjtalk_dictionary_manager.c
  )
endif()


# Configure RPATH for macOS
if(APPLE)
  set_target_properties(piper PROPERTIES
    MACOSX_RPATH TRUE
    INSTALL_RPATH "@executable_path/../lib;@loader_path/../lib;${PIPER_PHONEMIZE_DIR}/lib"
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${PIPER_PHONEMIZE_DIR}/lib"
  )
  set_target_properties(test_piper PROPERTIES
    MACOSX_RPATH TRUE
    INSTALL_RPATH "@executable_path/../lib;@loader_path/../lib;${PIPER_PHONEMIZE_DIR}/lib"
    BUILD_WITH_INSTALL_RPATH FALSE
    BUILD_RPATH "${PIPER_PHONEMIZE_DIR}/lib"
  )
endif()

# NOTE: external project prefix are shortened because of path length restrictions on Windows
# NOTE: onnxruntime is pulled from piper-phonemize

# ---- fmt ---

if(NOT DEFINED FMT_DIR)
  set(FMT_VERSION "10.0.0")
  set(FMT_DIR "${CMAKE_CURRENT_BINARY_DIR}/fi")

  include(ExternalProject)
  # Pass architecture settings to external projects
  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    set(EXTERNAL_CMAKE_ARGS -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES})
  else()
    set(EXTERNAL_CMAKE_ARGS "")
  endif()
  
  # Ensure MSVC runtime library is propagated to all external projects
  if(MSVC)
    list(APPEND EXTERNAL_CMAKE_ARGS
      -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
      -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
    )
  endif()

  ExternalProject_Add(
    fmt_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/f"
    URL "https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${FMT_DIR}
    CMAKE_ARGS -DFMT_TEST:BOOL=OFF  # Don't build all the tests
    CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=OFF  # Build static libraries
    CMAKE_ARGS ${EXTERNAL_CMAKE_ARGS}
  )
  add_dependencies(piper fmt_external)
  add_dependencies(test_piper fmt_external)
endif()

# ---- spdlog ---

if(NOT DEFINED SPDLOG_DIR)
  set(SPDLOG_DIR "${CMAKE_CURRENT_BINARY_DIR}/si")
  set(SPDLOG_VERSION "1.12.0")
  ExternalProject_Add(
    spdlog_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/s"
    URL "https://github.com/gabime/spdlog/archive/refs/tags/v${SPDLOG_VERSION}.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_DIR}
    CMAKE_ARGS -DSPDLOG_BUILD_SHARED:BOOL=OFF  # Build static libraries
    CMAKE_ARGS ${EXTERNAL_CMAKE_ARGS}
  )
  add_dependencies(piper spdlog_external)
  add_dependencies(test_piper spdlog_external)
endif()

# ---- piper-phonemize ---

if(NOT DEFINED PIPER_PHONEMIZE_DIR)
  set(PIPER_PHONEMIZE_DIR "${CMAKE_CURRENT_BINARY_DIR}/pi")
  # On Windows, we need espeak-ng as a shared library for proper functioning
  if(WIN32)
    set(PIPER_PHONEMIZE_SHARED_LIBS ON)
  else()
    set(PIPER_PHONEMIZE_SHARED_LIBS OFF)
  endif()
  
  ExternalProject_Add(
    piper_phonemize_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/p"
    URL "https://github.com/rhasspy/piper-phonemize/archive/refs/heads/master.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${PIPER_PHONEMIZE_DIR}
    CMAKE_ARGS -DBUILD_SHARED_LIBS:BOOL=${PIPER_PHONEMIZE_SHARED_LIBS}
    CMAKE_ARGS -DESPEAK_NG_BUILD_SHARED:BOOL=${PIPER_PHONEMIZE_SHARED_LIBS}
    CMAKE_ARGS ${EXTERNAL_CMAKE_ARGS}
  )
  add_dependencies(piper piper_phonemize_external)
  add_dependencies(test_piper piper_phonemize_external)
endif()

# Set runtime library for targets
if(MSVC)
  set_property(TARGET piper PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set_property(TARGET test_piper PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---- ONNX Runtime for Windows ---
if(WIN32)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_onnxruntime_windows.cmake)
endif()

# ---- HTSEngine (Unix platforms only) ---

if(NOT WIN32 AND NOT MSVC)
  if(NOT DEFINED HTS_ENGINE_DIR)
    set(HTS_ENGINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/he")
    set(HTS_ENGINE_VERSION "1.10")

    # Set architecture flags for configure scripts
    if(APPLE AND CMAKE_OSX_ARCHITECTURES)
      if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        set(CONFIGURE_HOST "--host=x86_64-apple-darwin")
      elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(CONFIGURE_HOST "--host=aarch64-apple-darwin")
      else()
        set(CONFIGURE_HOST "")
      endif()
      set(CONFIGURE_ENV "CFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES}" "CXXFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES}" "LDFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES}")
    else()
      set(CONFIGURE_HOST "")
      set(CONFIGURE_ENV "")
    endif()

    ExternalProject_Add(
      hts_engine_external
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/h"
      URL "https://downloads.sourceforge.net/project/hts-engine/hts_engine%20API/hts_engine_API-${HTS_ENGINE_VERSION}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz"
      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${CONFIGURE_ENV} ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./configure --prefix=${HTS_ENGINE_DIR} ${CONFIGURE_HOST}
      BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make
      INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make install
      BUILD_IN_SOURCE 1
      BUILD_BYPRODUCTS ${HTS_ENGINE_DIR}/lib/libHTSEngine.a
    )
  endif()

  # ---- OpenJTalk ---

  if(NOT DEFINED OPENJTALK_DIR)
    set(OPENJTALK_DIR "${CMAKE_CURRENT_BINARY_DIR}/oj")
    set(OPENJTALK_VERSION "1.11")

    ExternalProject_Add(
      openjtalk_external
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/o"
      URL "https://downloads.sourceforge.net/project/open-jtalk/Open%20JTalk/open_jtalk-${OPENJTALK_VERSION}/open_jtalk-${OPENJTALK_VERSION}.tar.gz"
      PATCH_COMMAND ""
      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${CONFIGURE_ENV} ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./configure
        --prefix=${OPENJTALK_DIR}
        --with-hts-engine-header-path=${HTS_ENGINE_DIR}/include
        --with-hts-engine-library-path=${HTS_ENGINE_DIR}/lib
        --with-charset=UTF-8
        ${CONFIGURE_HOST}
      BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make
      INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make install
      BUILD_IN_SOURCE 1
      BUILD_BYPRODUCTS
        ${OPENJTALK_DIR}/bin/open_jtalk
      DEPENDS hts_engine_external
    )
  endif()
endif()

# ---- Declare executable ----

if(APPLE)
  # macOS-specific settings
  set(PIPER_EXTRA_LIBRARIES "pthread")
  # Find Homebrew-installed espeak-ng
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(HOMEBREW_PREFIX "/usr/local")
  else()
    set(HOMEBREW_PREFIX "/opt/homebrew")
  endif()
elseif(NOT MSVC)
  # Linux flags
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
  target_link_libraries(piper -static-libgcc -static-libstdc++)

  set(PIPER_EXTRA_LIBRARIES "pthread")
endif()

# Platform-specific library linking
if(WIN32)
  # Windows: Use conditional linking for libraries
  find_library(FMT_LIB NAMES fmt 
    PATHS ${FMT_DIR}/lib 
    NO_DEFAULT_PATH)
  find_library(SPDLOG_LIB NAMES spdlog 
    PATHS ${SPDLOG_DIR}/lib 
    NO_DEFAULT_PATH)
  find_library(PIPER_PHONEMIZE_LIB NAMES piper_phonemize 
    PATHS ${PIPER_PHONEMIZE_DIR}/lib 
    NO_DEFAULT_PATH)
  find_library(ESPEAK_NG_LIB NAMES espeak-ng 
    PATHS ${PIPER_PHONEMIZE_DIR}/lib 
    NO_DEFAULT_PATH)
  
  target_link_libraries(piper
    $<IF:$<BOOL:${FMT_LIB}>,${FMT_LIB},fmt>
    $<IF:$<BOOL:${SPDLOG_LIB}>,${SPDLOG_LIB},spdlog>
    $<IF:$<BOOL:${PIPER_PHONEMIZE_LIB}>,${PIPER_PHONEMIZE_LIB},piper_phonemize>
    $<IF:$<BOOL:${ESPEAK_NG_LIB}>,${ESPEAK_NG_LIB},espeak-ng>
    $<IF:$<BOOL:${ONNXRUNTIME_LIB}>,${ONNXRUNTIME_LIB},onnxruntime>
    ${PIPER_EXTRA_LIBRARIES}
  )
else()
  # Unix: Use standard library names
  target_link_libraries(piper
    fmt
    spdlog
    piper_phonemize
    espeak-ng
    onnxruntime
    ${PIPER_EXTRA_LIBRARIES}
  )
endif()

# Link OpenJTalk and HTS_Engine libraries
if(NOT WIN32 AND NOT MSVC)
  # Ensure libraries are built before linking
  add_dependencies(piper openjtalk_external hts_engine_external)
  add_dependencies(test_piper openjtalk_external hts_engine_external)

  # Link libraries with absolute paths
  # Note: OpenJTalk doesn't create a single library, we need to link individual components
  target_link_libraries(piper
    ${CMAKE_CURRENT_BINARY_DIR}/he/lib/libHTSEngine.a
  )
  target_link_libraries(test_piper
    ${CMAKE_CURRENT_BINARY_DIR}/he/lib/libHTSEngine.a
  )
endif()

target_link_directories(piper PUBLIC
  ${FMT_DIR}/lib
  ${SPDLOG_DIR}/lib
  ${PIPER_PHONEMIZE_DIR}/lib
)

# Note: We don't need Homebrew paths for espeak-ng on macOS
# because we're using the custom build from piper_phonemize

target_include_directories(piper PUBLIC
  ${FMT_DIR}/include
  ${SPDLOG_DIR}/include
  ${PIPER_PHONEMIZE_DIR}/include
)

# Add ONNX Runtime include directory for Windows
if(WIN32 AND ONNXRUNTIME_INCLUDE_DIR)
  target_include_directories(piper PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
  target_include_directories(test_piper PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
endif()

if(NOT WIN32 AND NOT MSVC)
  target_include_directories(piper PUBLIC
    ${OPENJTALK_DIR}/include
    ${HTS_ENGINE_DIR}/include
  )
  target_include_directories(test_piper PUBLIC
    ${OPENJTALK_DIR}/include
    ${HTS_ENGINE_DIR}/include
  )
endif()

# OpenJTalk is used via binary execution, includes not needed

target_compile_definitions(piper PUBLIC _PIPER_VERSION=${piper_version})

# Add OpenJTalk dictionary path definition on Unix platforms
if(NOT WIN32 AND NOT MSVC)
  # Dictionary will be downloaded by CI/CD or provided by user
  # Use relative path from binary location for better portability
  target_compile_definitions(piper PUBLIC OPENJTALK_DIC_PATH="../share/open_jtalk/dic")
  target_compile_definitions(test_piper PUBLIC OPENJTALK_DIC_PATH="${CMAKE_CURRENT_BINARY_DIR}/naist-jdic")
endif()

# ---- Declare test ----
include(CTest)
enable_testing()
add_test(
  NAME test_piper
  COMMAND test_piper "${CMAKE_SOURCE_DIR}/etc/test_voice.onnx" "${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data" "${CMAKE_CURRENT_BINARY_DIR}/test.wav"
)

target_compile_features(test_piper PUBLIC cxx_std_17)

target_include_directories(
  test_piper PUBLIC
  ${FMT_DIR}/include
  ${SPDLOG_DIR}/include
  ${PIPER_PHONEMIZE_DIR}/include
)

target_link_directories(
  test_piper PUBLIC
  ${FMT_DIR}/lib
  ${SPDLOG_DIR}/lib
  ${PIPER_PHONEMIZE_DIR}/lib
)

# Platform-specific library linking for test_piper
if(WIN32)
  target_link_libraries(test_piper
    $<IF:$<BOOL:${FMT_LIB}>,${FMT_LIB},fmt>
    $<IF:$<BOOL:${SPDLOG_LIB}>,${SPDLOG_LIB},spdlog>
    $<IF:$<BOOL:${PIPER_PHONEMIZE_LIB}>,${PIPER_PHONEMIZE_LIB},piper_phonemize>
    $<IF:$<BOOL:${ESPEAK_NG_LIB}>,${ESPEAK_NG_LIB},espeak-ng>
    $<IF:$<BOOL:${ONNXRUNTIME_LIB}>,${ONNXRUNTIME_LIB},onnxruntime>
    ${PIPER_EXTRA_LIBRARIES}
  )
else()
  target_link_libraries(test_piper
    fmt
    spdlog
    espeak-ng
    piper_phonemize
    onnxruntime
    ${PIPER_EXTRA_LIBRARIES}
  )
endif()

# OpenJTalk is used via binary execution for test_piper, not linked

# ---- Declare install targets ----

install(
  TARGETS piper
  DESTINATION bin)

# Dependencies
install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/bin/
  DESTINATION bin
  USE_SOURCE_PERMISSIONS  # keep +x
  FILES_MATCHING
  PATTERN "piper_phonemize"
  PATTERN "espeak-ng"
  PATTERN "*.dll"
)

install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/lib/
  DESTINATION lib
  FILES_MATCHING
  PATTERN "*.dll"
  PATTERN "*.so*"
  PATTERN "*.dylib"
)

install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data
  DESTINATION share
)

install(
  FILES ${PIPER_PHONEMIZE_DIR}/share/libtashkeel_model.ort
  DESTINATION share
)

# Install OpenJTalk for Japanese TTS support
if(NOT WIN32 AND NOT MSVC)
  # Install OpenJTalk binary
  install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/oj/bin/open_jtalk
    DESTINATION bin
    OPTIONAL  # Don't fail if not built
  )

  # Install OpenJTalk dictionary
  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/share/open_jtalk
    DESTINATION share
    OPTIONAL  # Don't fail if not present
  )
endif()

# Note: We don't need to copy espeak-ng library on macOS separately
# because piper_phonemize builds and installs its own custom fork
# of espeak-ng that includes the required espeak_TextToPhonemesWithTerminator
# function. The library is already installed by the piper_phonemize
# external project.

# ---- Windows-specific DLL handling ----
if(WIN32)
  # Copy ONNX Runtime DLL to output directory after build
  if(ONNXRUNTIME_DLL)
    add_custom_command(TARGET piper POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_DLL}"
        "$<TARGET_FILE_DIR:piper>/onnxruntime.dll"
      COMMENT "Copying ONNX Runtime DLL..."
    )
    
    add_custom_command(TARGET test_piper POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_DLL}"
        "$<TARGET_FILE_DIR:test_piper>/onnxruntime.dll"
      COMMENT "Copying ONNX Runtime DLL for tests..."
    )
    
    # Install ONNX Runtime DLL
    install(FILES ${ONNXRUNTIME_DLL} DESTINATION bin)
  endif()
  
  # Copy all DLLs from piper-phonemize to output directories
  add_custom_command(TARGET piper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PIPER_PHONEMIZE_DIR}/bin"
      "$<TARGET_FILE_DIR:piper>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PIPER_PHONEMIZE_DIR}/lib"
      "$<TARGET_FILE_DIR:piper>"
    COMMENT "Copying piper-phonemize DLLs..."
  )
  
  add_custom_command(TARGET test_piper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PIPER_PHONEMIZE_DIR}/bin"
      "$<TARGET_FILE_DIR:test_piper>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PIPER_PHONEMIZE_DIR}/lib"
      "$<TARGET_FILE_DIR:test_piper>"
    COMMENT "Copying piper-phonemize DLLs for tests..."
  )
  
  # Find and copy espeak-ng DLL if it exists
  find_file(ESPEAK_NG_DLL
    NAMES espeak-ng.dll libespeak-ng.dll
    PATHS
      "${PIPER_PHONEMIZE_DIR}/bin"
      "${PIPER_PHONEMIZE_DIR}/lib"
      "${CMAKE_CURRENT_BINARY_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build"
      "${CMAKE_CURRENT_BINARY_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build/src"
    NO_DEFAULT_PATH
  )
  
  if(ESPEAK_NG_DLL)
    add_custom_command(TARGET piper POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ESPEAK_NG_DLL}"
        "$<TARGET_FILE_DIR:piper>/"
      COMMENT "Copying espeak-ng DLL..."
    )
    add_custom_command(TARGET test_piper POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ESPEAK_NG_DLL}"
        "$<TARGET_FILE_DIR:test_piper>/"
      COMMENT "Copying espeak-ng DLL for tests..."
    )
  endif()
  
  # Copy Microsoft C++ runtime redistributables if needed
  # These are sometimes required when using static runtime linking
  set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
  include(InstallRequiredSystemLibraries)
  
  # Also copy espeak-ng data for test
  add_custom_command(TARGET test_piper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data"
      "$<TARGET_FILE_DIR:test_piper>/espeak-ng-data"
    COMMENT "Copying espeak-ng data for tests..."
  )
endif()