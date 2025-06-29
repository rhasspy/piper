# Find or download ONNX Runtime for Windows
if(WIN32)
  set(ONNXRUNTIME_VERSION "1.14.1")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ONNX_ARCH "x64")
  else()
    set(ONNX_ARCH "x86")
  endif()
  
  # First try to find ONNX Runtime from piper-phonemize build
  set(ONNXRUNTIME_SEARCH_PATHS
    "${PIPER_PHONEMIZE_DIR}/lib"
    "${CMAKE_CURRENT_BINARY_DIR}/p/src/piper_phonemize_external-build/_deps/onnxruntime-src/lib"
    "${CMAKE_CURRENT_BINARY_DIR}/_deps/onnxruntime-src/lib"
  )
  
  # Search for ONNX Runtime library
  find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    PATHS ${ONNXRUNTIME_SEARCH_PATHS}
    NO_DEFAULT_PATH
  )
  
  # Search for include directory
  find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATH_SUFFIXES include include/onnxruntime include/onnxruntime/core/session
    PATHS 
      "${PIPER_PHONEMIZE_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}/p/src/piper_phonemize_external-build/_deps/onnxruntime-src"
      "${CMAKE_CURRENT_BINARY_DIR}/_deps/onnxruntime-src"
    NO_DEFAULT_PATH
  )
  
  # If not found, download it
  if(NOT ONNXRUNTIME_LIB OR NOT ONNXRUNTIME_INCLUDE_DIR)
    message(STATUS "ONNX Runtime not found, downloading...")
    
    set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-win-${ONNX_ARCH}-${ONNXRUNTIME_VERSION}.zip")
    set(ONNXRUNTIME_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/onnxruntime")
    set(ONNXRUNTIME_ZIP "${ONNXRUNTIME_DOWNLOAD_DIR}/onnxruntime.zip")
    
    # Create download directory
    file(MAKE_DIRECTORY ${ONNXRUNTIME_DOWNLOAD_DIR})
    
    # Download
    if(NOT EXISTS "${ONNXRUNTIME_ZIP}")
      message(STATUS "Downloading ONNX Runtime from: ${ONNXRUNTIME_URL}")
      file(DOWNLOAD
        ${ONNXRUNTIME_URL}
        ${ONNXRUNTIME_ZIP}
        SHOW_PROGRESS
        STATUS download_status
        TIMEOUT 300
      )
      
      list(GET download_status 0 status_code)
      if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download ONNX Runtime")
      endif()
    endif()
    
    # Extract
    if(EXISTS "${ONNXRUNTIME_ZIP}")
      message(STATUS "Extracting ONNX Runtime...")
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${ONNXRUNTIME_ZIP}"
        WORKING_DIRECTORY "${ONNXRUNTIME_DOWNLOAD_DIR}"
        RESULT_VARIABLE extract_result
      )
      
      if(extract_result EQUAL 0)
        set(ONNXRUNTIME_ROOT "${ONNXRUNTIME_DOWNLOAD_DIR}/onnxruntime-win-${ONNX_ARCH}-${ONNXRUNTIME_VERSION}")
        set(ONNXRUNTIME_LIB "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib")
        set(ONNXRUNTIME_DLL "${ONNXRUNTIME_ROOT}/lib/onnxruntime.dll")
        set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT}/include")
        
        # Set variables for parent scope
        set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB} PARENT_SCOPE)
        set(ONNXRUNTIME_DLL ${ONNXRUNTIME_DLL} PARENT_SCOPE)
        set(ONNXRUNTIME_INCLUDE_DIR ${ONNXRUNTIME_INCLUDE_DIR} PARENT_SCOPE)
      else()
        message(FATAL_ERROR "Failed to extract ONNX Runtime")
      endif()
    endif()
  else()
    # Found in piper-phonemize, set DLL path
    get_filename_component(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_LIB}" DIRECTORY)
    set(ONNXRUNTIME_DLL "${ONNXRUNTIME_LIB_DIR}/onnxruntime.dll" PARENT_SCOPE)
    set(ONNXRUNTIME_LIB ${ONNXRUNTIME_LIB} PARENT_SCOPE)
    set(ONNXRUNTIME_INCLUDE_DIR ${ONNXRUNTIME_INCLUDE_DIR} PARENT_SCOPE)
  endif()
  
  message(STATUS "ONNX Runtime library: ${ONNXRUNTIME_LIB}")
  message(STATUS "ONNX Runtime DLL: ${ONNXRUNTIME_DLL}")
  message(STATUS "ONNX Runtime include: ${ONNXRUNTIME_INCLUDE_DIR}")
endif()