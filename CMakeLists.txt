cmake_minimum_required(VERSION 3.13)

project(piper C CXX)

file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" piper_version)
string(STRIP "${piper_version}" piper_version)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Force compiler to use UTF-8 for IPA constants
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
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

# Add OpenJTalk support on Unix platforms (not Windows)
if(NOT WIN32 AND NOT MSVC)
  target_sources(piper PRIVATE 
    src/cpp/openjtalk_wrapper.c
  )
  target_sources(test_piper PRIVATE 
    src/cpp/openjtalk_wrapper.c
  )
endif()

# Configure RPATH for macOS
if(APPLE)
  set_target_properties(piper PROPERTIES
    MACOSX_RPATH TRUE
    INSTALL_RPATH "@executable_path/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  set_target_properties(test_piper PROPERTIES
    MACOSX_RPATH TRUE
    INSTALL_RPATH "@executable_path/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
endif()

# NOTE: external project prefix are shortened because of path length restrictions on Windows
# NOTE: onnxruntime is pulled from piper-phonemize

# ---- fmt ---

if(NOT DEFINED FMT_DIR)
  set(FMT_VERSION "10.0.0")
  set(FMT_DIR "${CMAKE_CURRENT_BINARY_DIR}/fi")

  include(ExternalProject)
  ExternalProject_Add(
    fmt_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/f"
    URL "https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${FMT_DIR}
    CMAKE_ARGS -DFMT_TEST:BOOL=OFF  # Don't build all the tests
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
  )
  add_dependencies(piper spdlog_external)
  add_dependencies(test_piper spdlog_external)
endif()

# ---- piper-phonemize ---

if(NOT DEFINED PIPER_PHONEMIZE_DIR)
  set(PIPER_PHONEMIZE_DIR "${CMAKE_CURRENT_BINARY_DIR}/pi")
  ExternalProject_Add(
    piper_phonemize_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/p"
    URL "https://github.com/rhasspy/piper-phonemize/archive/refs/heads/master.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${PIPER_PHONEMIZE_DIR}
  )
  add_dependencies(piper piper_phonemize_external)
  add_dependencies(test_piper piper_phonemize_external)
endif()

# ---- HTSEngine (Unix platforms only) ---

if(NOT WIN32 AND NOT MSVC)
  if(NOT DEFINED HTS_ENGINE_DIR)
    set(HTS_ENGINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/he")
    set(HTS_ENGINE_VERSION "1.10")
    
    ExternalProject_Add(
      hts_engine_external
      PREFIX "${CMAKE_CURRENT_BINARY_DIR}/h"
      URL "https://downloads.sourceforge.net/project/hts-engine/hts_engine%20API/hts_engine_API-${HTS_ENGINE_VERSION}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz"
      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./configure --prefix=${HTS_ENGINE_DIR}
      BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make
      INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make install
      BUILD_IN_SOURCE 1
      BUILD_BYPRODUCTS ${HTS_ENGINE_DIR}/lib/libHTSEngine.a
    )
    add_dependencies(piper hts_engine_external)
    add_dependencies(test_piper hts_engine_external)
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
      CONFIGURE_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./configure 
        --prefix=${OPENJTALK_DIR}
        --with-hts-engine-header-path=${HTS_ENGINE_DIR}/include
        --with-hts-engine-library-path=${HTS_ENGINE_DIR}/lib
        --with-charset=UTF-8
      BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make
      INSTALL_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> make install
      BUILD_IN_SOURCE 1
      BUILD_BYPRODUCTS 
        ${OPENJTALK_DIR}/lib/libopenjtalk.a
        ${OPENJTALK_DIR}/bin/open_jtalk
      DEPENDS hts_engine_external
    )
    add_dependencies(piper openjtalk_external)
    add_dependencies(test_piper openjtalk_external)
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

target_link_libraries(piper
  fmt
  spdlog
  piper_phonemize
  onnxruntime
  ${PIPER_EXTRA_LIBRARIES}
)

# Link espeak-ng
target_link_libraries(piper espeak-ng)

# OpenJTalk is used via binary execution, not linked

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

# OpenJTalk is used via binary execution, includes not needed

target_compile_definitions(piper PUBLIC _PIPER_VERSION=${piper_version})

# Add OpenJTalk dictionary path definition on Unix platforms
if(NOT WIN32 AND NOT MSVC)
  # Dictionary will be downloaded by CI/CD or provided by user
  # For testing, use build directory path
  target_compile_definitions(piper PUBLIC OPENJTALK_DIC_PATH="${CMAKE_INSTALL_PREFIX}/share/open_jtalk/dic")
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

target_link_libraries(test_piper
  fmt
  spdlog
  espeak-ng
  piper_phonemize
  onnxruntime
)

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

# Note: We don't need to copy espeak-ng library on macOS separately
# because piper_phonemize builds and installs its own custom fork
# of espeak-ng that includes the required espeak_TextToPhonemesWithTerminator
# function. The library is already installed by the piper_phonemize
# external project.