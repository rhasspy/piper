cmake_minimum_required(VERSION 3.13)

project(piper C CXX)

file(READ "${CMAKE_CURRENT_LIST_DIR}/VERSION" piper_version)
# 末尾の改行がコンパイラ定義に混入しないようにトリム
string(STRIP "${piper_version}" piper_version)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Force compiler to use UTF-8 for IPA constants
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  
  # Windows固有の定義
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  add_definitions(-DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 or later
  
  # ONNX Runtimeのヘッダーファイルの問題を解決
  add_definitions(-D_Frees_ptr_opt_=__declspec(dllexport))
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DWIN32)
  add_definitions(-D_WINDOWS)
  add_definitions(-D_UNICODE)
  add_definitions(-DUNICODE)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-D_WIN32_WINNT=0x0601)
  add_definitions(-DWINVER=0x0601)
  add_definitions(-D_WIN32_IE=0x0601)
  add_definitions(-D_WIN32_WINDOWS=0x0601)
  
  # MSVC固有のコンパイラオプション
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /MP /bigobj")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /RTC1")
  
  # コンパイルオプションの修正
  add_compile_options(/W4 /WX /wd4100 /wd4505 /wd4996)
  add_compile_options(/permissive)  # /permissive-を/permissiveに変更
  add_compile_options(/Zc:__cplusplus)
  add_compile_options(/Zc:inline)
  add_compile_options(/Zc:strictStrings)
  add_compile_options(/Zc:throwingNew)
  add_compile_options(/Zc:referenceBinding)
  add_compile_options(/Zc:rvalueCast)
  add_compile_options(/bigobj)  # 大きなオブジェクトファイルをサポート
  add_compile_options(/MP)  # マルチプロセッサコンパイルを有効化
  add_compile_options(/wd4244)  # 型変換の警告を無視
  add_compile_options(/wd4267)  # size_tからintへの変換の警告を無視
  add_compile_options(/wd4996)  # 非推奨関数の警告を無視
  
  # ONNX Runtimeとの互換性のための追加定義
  add_definitions(-DONNXRUNTIME_DLL)
  add_definitions(-DONNXRUNTIME_STATIC_LIB)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  
  # Windows SDKのヘッダーを追加
  include_directories("$ENV{WindowsSdkDir}Include\\$ENV{WindowsSDKVersion}um")
  include_directories("$ENV{WindowsSdkDir}Include\\$ENV{WindowsSDKVersion}shared")
  include_directories("$ENV{WindowsSdkDir}Include\\$ENV{WindowsSDKVersion}ucrt")
  
  # リンクライブラリを追加
  link_libraries(winmm ws2_32 version)
  
  # Windows固有のソースファイルの設定
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/windows_console.cpp")
    target_sources(piper PRIVATE src/cpp/windows_console.cpp)
  endif()
elseif(NOT APPLE)
  # Linux flags
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
elseif(APPLE)
  # macOS
  string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
  string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")
endif()

add_executable(piper src/cpp/main.cpp src/cpp/piper.cpp)
add_executable(test_piper src/cpp/test.cpp src/cpp/piper.cpp)

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

# ---- Declare executable ----

if((NOT MSVC) AND (NOT APPLE))
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

# Windows固有のリンク設定
if(MSVC)
  target_link_libraries(piper PRIVATE
    winmm
    ws2_32
    version
  )
endif()

target_link_directories(piper PUBLIC
  ${FMT_DIR}/lib
  ${SPDLOG_DIR}/lib
  ${PIPER_PHONEMIZE_DIR}/lib
)

target_include_directories(piper PUBLIC
  ${FMT_DIR}/include
  ${SPDLOG_DIR}/include
  ${PIPER_PHONEMIZE_DIR}/include
)

target_compile_definitions(piper PUBLIC _PIPER_VERSION=${piper_version})

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
  RUNTIME DESTINATION bin
)

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
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  FILES_MATCHING
  PATTERN "*.dll"
  PATTERN "*.so*"
  PATTERN "*.dylib"
)

install(
  DIRECTORY ${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
  FILES ${PIPER_PHONEMIZE_DIR}/share/libtashkeel_model.ort
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

# Windows固有のターゲット設定
if(MSVC)
  target_compile_definitions(piper PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _UNICODE
    UNICODE
    _USE_MATH_DEFINES
    _WIN32_WINNT=0x0601
    WINVER=0x0601
    _WIN32_IE=0x0601
    _WIN32_WINDOWS=0x0601
    ONNXRUNTIME_DLL
    ONNXRUNTIME_STATIC_LIB
  )
  
  # Windows SDKのバージョン設定
  if(NOT DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.19041.0")
  endif()
  
  # ONNX Runtimeの設定
  target_compile_definitions(piper_phonemize PRIVATE
    ONNXRUNTIME_DLL
    ONNXRUNTIME_STATIC_LIB
    _Frees_ptr_opt_=__declspec(dllexport)
  )
endif()

install(
  DIRECTORY ${FMT_DIR}/lib/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  FILES_MATCHING PATTERN "*.dylib" PATTERN "*.so*"
)

install(
  DIRECTORY ${SPDLOG_DIR}/lib/
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  FILES_MATCHING PATTERN "*.dylib" PATTERN "*.so*"
)

# macOS 実行時に同梱 lib を検索させる
if(APPLE)
  set_target_properties(piper PROPERTIES
    INSTALL_RPATH "@loader_path/../lib")
endif()
