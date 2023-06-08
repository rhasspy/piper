cmake_minimum_required(VERSION 3.13)

include(CheckIncludeFileCXX)

project(piper C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_EXECUTABLE(piper main.cpp piper.cpp)

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -Wl,-rpath,'$ORIGIN'")
string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")

set(PIPER_PHONEMIZE_ROOTDIR ${CMAKE_CURRENT_LIST_DIR}/../../lib/${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}/piper_phonemize)
set(ONNXRUNTIME_ROOTDIR ${CMAKE_CURRENT_LIST_DIR}/../../lib/${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}/onnxruntime)

target_link_libraries(piper
                      piper_phonemize
                      espeak-ng
                      onnxruntime
                      pthread)

if(NOT APPLE)
  target_link_libraries(-static-libgcc -static-libstdc++)
endif()

target_link_directories(piper PUBLIC
                        ${PIPER_PHONEMIZE_ROOTDIR}/lib
                        ${ONNXRUNTIME_ROOTDIR}/lib)

target_include_directories(piper PUBLIC
                           ${PIPER_PHONEMIZE_ROOTDIR}/include
                           ${ONNXRUNTIME_ROOTDIR}/include)
