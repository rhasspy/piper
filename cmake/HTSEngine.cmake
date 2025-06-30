# HTSEngine Windows build configuration
# This file provides CMake-based build for HTSEngine on Windows

set(HTS_ENGINE_VERSION "1.10")
set(HTS_ENGINE_URL "https://downloads.sourceforge.net/project/hts-engine/hts_engine%20API/hts_engine_API-${HTS_ENGINE_VERSION}/hts_engine_API-${HTS_ENGINE_VERSION}.tar.gz")

if(WIN32)
  # Create a CMake-based build for HTSEngine on Windows
  set(HTS_ENGINE_DIR "${CMAKE_CURRENT_BINARY_DIR}/he")
  
  # Download and extract HTSEngine
  ExternalProject_Add(
    hts_engine_external
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}/h"
    URL ${HTS_ENGINE_URL}
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_PREFIX=${HTS_ENGINE_DIR}
      -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy 
      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/HTSEngine_CMakeLists.txt 
      <SOURCE_DIR>/CMakeLists.txt
    BUILD_BYPRODUCTS 
      ${HTS_ENGINE_DIR}/lib/HTSEngine.lib
      ${HTS_ENGINE_DIR}/lib/libHTSEngine.a
  )
endif()