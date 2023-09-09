cmake_minimum_required(VERSION 3.13)

project(piper C CXX)

file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" piper_version)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(piper src/cpp/main.cpp src/cpp/piper.cpp)

# NOTE: onnxruntime is pulled from piper-phonemize

# ---- fmt ---

if(NOT DEFINED FMT_DIR)
  set(FMT_VERSION "10.0.0")
  set(FMT_DIR "${CMAKE_CURRENT_BINARY_DIR}/fmt_install")

  include(ExternalProject)
  ExternalProject_Add(
    fmt_external
    URL "https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${FMT_DIR}
    CMAKE_ARGS -DFMT_TEST:BOOL=OFF  # Don't build all the tests
  )
  add_dependencies(piper fmt_external)
endif()

# ---- spdlog ---

if(NOT DEFINED SPDLOG_DIR)
  set(SPDLOG_DIR "${CMAKE_CURRENT_BINARY_DIR}/spdlog_install")
  set(SPDLOG_VERSION "1.12.0")
  ExternalProject_Add(
    spdlog_external
    URL "https://github.com/gabime/spdlog/archive/refs/tags/v${SPDLOG_VERSION}.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_DIR}
  )
  add_dependencies(piper spdlog_external)
endif()

# ---- piper-phonemize ---

if(NOT DEFINED PIPER_PHONEMIZE_DIR)
  set(PIPER_PHONEMIZE_DIR "${CMAKE_CURRENT_BINARY_DIR}/piper_phonemize_install")
  ExternalProject_Add(
    piper_phonemize_external
    URL "https://github.com/rhasspy/piper-phonemize/archive/refs/heads/workflow.zip"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${PIPER_PHONEMIZE_DIR}
  )
  add_dependencies(piper piper_phonemize_external)
endif()

# ---- Declare executable ----

if(WIN32)
  # Force compiler to use UTF-8 for IPA constants
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
elseif(NOT APPLE)
  # Linux flags
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
  target_link_libraries(piper -static-libgcc -static-libstdc++)

  set(PIPER_EXTRA_LIBRARIES "pthread")
endif()

target_link_libraries(piper
  fmt
  spdlog
  espeak-ng
  piper_phonemize
  onnxruntime
  ${PIPER_EXTRA_LIBRARIES}
)

target_link_directories(piper PUBLIC
  ${FMT_DIR}/lib
  ${SPDLOG_DIR}/lib
  ${PIPER_PHONEMIZE_DIR}/lib
  ${ONNXRUNTIME_DIR}/lib
)

target_include_directories(piper PUBLIC
  ${FMT_DIR}/include
  ${SPDLOG_DIR}/include
  ${PIPER_PHONEMIZE_DIR}/include
  ${ONNXRUNTIME_DIR}/include
)

target_compile_definitions(piper PUBLIC _PIPER_VERSION=${piper_version})

# ---- Declare test ----

include(CTest)
enable_testing()
add_executable(test_piper src/cpp/test.cpp src/cpp/piper.cpp)
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
  ${ONNXRUNTIME_DIR}/include
)

target_link_directories(
  test_piper PUBLIC
  ${FMT_DIR}/lib
  ${SPDLOG_DIR}/lib
  ${PIPER_PHONEMIZE_DIR}/lib
  ${ONNXRUNTIME_DIR}/lib
)

target_link_libraries(test_piper PUBLIC
  fmt
  spdlog
  espeak-ng
  piper_phonemize
  onnxruntime
)

# ---- Declare install targets ----

install(
  TARGETS piper
  DESTINATION ${CMAKE_INSTALL_PREFIX})

# Dependencies
install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/bin/
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  USE_SOURCE_PERMISSIONS  # keep +x
  FILES_MATCHING
  PATTERN "piper_phonemize"
  PATTERN "espeak-ng"
  PATTERN "*.dll"
)

install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/lib/
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  FILES_MATCHING
  PATTERN "*.dll"
  PATTERN "*.so*"
)

install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
  FILES ${PIPER_PHONEMIZE_DIR}/share/libtashkeel_model.ort
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)
