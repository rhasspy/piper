cmake_minimum_required(VERSION 3.13)

include(CheckIncludeFileCXX)

project(piper C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_EXECUTABLE(piper main.cpp)

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")

find_package(PkgConfig)
pkg_check_modules(ESPEAK_NG REQUIRED espeak-ng<2)

# https://github.com/espeak-ng/pcaudiolib
check_include_file_cxx("pcaudiolib/audio.h" PCAUDIO_INCLUDE_FOUND)

if(PCAUDIO_INCLUDE_FOUND)
  option(USE_PCAUDIO "Build with pcaudiolib" ON)
  if(USE_PCAUDIO)
    target_compile_definitions(piper PUBLIC HAVE_PCAUDIO)
    set(PCAUDIO_LIBRARIES "pcaudio")
  endif()
endif()

set(ONNXRUNTIME_ROOTDIR ${CMAKE_CURRENT_LIST_DIR}/../../lib/${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR})

target_link_libraries(piper
                      onnxruntime
                      -static-libgcc -static-libstdc++
                      ${ESPEAK_NG_LIBRARIES}
                      ${PCAUDIO_LIBRARIES})

target_link_directories(piper PUBLIC
                        ${ESPEAK_NG_LIBRARY_DIRS}
                        ${ONNXRUNTIME_ROOTDIR}/lib)

target_include_directories(piper PUBLIC
                           ${ONNXRUNTIME_ROOTDIR}/include
                           ${ESPEAK_NG_INCLUDE_DIRS})

target_compile_options(piper PUBLIC
                       ${ESPEAK_NG_CFLAGS_OTHER})
