cmake_minimum_required(VERSION 3.13)

include(CheckIncludeFileCXX)

project(larynx C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ADD_EXECUTABLE(larynx main.cpp)

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
string(APPEND CMAKE_C_FLAGS " -Wall -Wextra")

find_package(PkgConfig)
pkg_check_modules(ESPEAK_NG REQUIRED espeak-ng<2)

# https://github.com/espeak-ng/pcaudiolib
check_include_file_cxx("pcaudiolib/audio.h" PCAUDIO_INCLUDE_FOUND)
if(PCAUDIO_INCLUDE_FOUND)
target_compile_definitions(larynx PUBLIC HAVE_PCAUDIO)
set(PCAUDIO_LIBRARIES "pcaudio")
endif()

find_package(Boost 1.3.0 REQUIRED COMPONENTS program_options)

set(ONNXRUNTIME_ROOTDIR "/usr/local/include/onnxruntime")

target_link_libraries(larynx
                      onnxruntime
                      ${ESPEAK_NG_LIBRARIES}
                      ${PCAUDIO_LIBRARIES}
                      Boost::program_options)

target_link_directories(larynx PUBLIC
                        ${ONNXRUNTIME_ROOTDIR}/lib)

target_include_directories(larynx PUBLIC
                           ${ONNXRUNTIME_ROOTDIR}/include
                           ${ESPEAK_NG_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIRS})

target_compile_options(larynx PUBLIC
                       ${ESPEAK_NG_CFLAGS_OTHER})
