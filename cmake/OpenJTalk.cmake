# OpenJTalk Windows build configuration
# This file provides CMake-based build for OpenJTalk on Windows

set(OPENJTALK_VERSION "1.11")
set(OPENJTALK_URL "https://downloads.sourceforge.net/project/open-jtalk/Open%20JTalk/open_jtalk-${OPENJTALK_VERSION}/open_jtalk-${OPENJTALK_VERSION}.tar.gz")

if(WIN32)
  # Create a CMake-based build for OpenJTalk on Windows
  set(OPENJTALK_DIR "${CMAKE_CURRENT_BINARY_DIR}/oj")
  
  # Ensure HTSEngine is built first
  if(NOT TARGET hts_engine_external)
    message(FATAL_ERROR "HTSEngine must be built before OpenJTalk")
  endif()
  
  # Download and build OpenJTalk
  ExternalProject_Add(
    openjtalk_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/o"
    URL ${OPENJTALK_URL}
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_PREFIX=${OPENJTALK_DIR}
      -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
      -DHTS_ENGINE_INCLUDE_DIR=${HTS_ENGINE_DIR}/include
      -DHTS_ENGINE_LIB=${HTS_ENGINE_DIR}/lib/HTSEngine.lib
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy 
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenJTalk_CMakeLists.txt 
      <SOURCE_DIR>/CMakeLists.txt
    BUILD_BYPRODUCTS 
      ${OPENJTALK_DIR}/bin/open_jtalk.exe
      ${OPENJTALK_DIR}/lib/libopenjtalk.lib
      ${OPENJTALK_DIR}/lib/libopenjtalk.a
    DEPENDS hts_engine_external
  )
endif()