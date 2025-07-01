# Copy DLLs script for test_piper executable
# This script is executed at build time to copy necessary DLLs

# Get the directories from cache variables
set(PIPER_PHONEMIZE_DIR "${CMAKE_CURRENT_BINARY_DIR}/pi")
set(PIPER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Get target directory
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(TARGET_DIR "${PIPER_BUILD_DIR}/Debug")
else()
  set(TARGET_DIR "${PIPER_BUILD_DIR}/Release")
endif()

# Create list of DLL search paths
set(DLL_SEARCH_PATHS
  "${PIPER_PHONEMIZE_DIR}/bin"
  "${PIPER_PHONEMIZE_DIR}/lib"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/Release"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/Debug"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build/Release"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build/Debug"
)

# Essential DLLs to copy
set(ESSENTIAL_DLLS
  "espeak-ng.dll"
  "piper_phonemize.dll"
)

# Copy essential DLLs
foreach(dll_name ${ESSENTIAL_DLLS})
  set(dll_found FALSE)
  foreach(search_path ${DLL_SEARCH_PATHS})
    if(EXISTS "${search_path}/${dll_name}")
      message("Copying ${dll_name} from ${search_path} for test_piper")
      file(COPY "${search_path}/${dll_name}" DESTINATION "${TARGET_DIR}")
      set(dll_found TRUE)
      break()
    endif()
  endforeach()
  if(NOT dll_found)
    message(WARNING "Could not find ${dll_name} for test_piper")
  endif()
endforeach()

# Copy all DLLs from piper-phonemize directories
foreach(search_path ${DLL_SEARCH_PATHS})
  if(EXISTS "${search_path}")
    file(GLOB dlls "${search_path}/*.dll")
    foreach(dll ${dlls})
      get_filename_component(dll_name ${dll} NAME)
      message("Copying ${dll_name} for test_piper")
      file(COPY ${dll} DESTINATION "${TARGET_DIR}")
    endforeach()
  endif()
endforeach()

# Also copy espeak-ng-data
set(ESPEAK_DATA_SEARCH_PATHS
  "${PIPER_PHONEMIZE_DIR}/share/espeak-ng-data"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/_deps/espeak_ng-build/espeak-ng-data"
  "${PIPER_BUILD_DIR}/p/src/piper_phonemize_external-build/espeak-ng-data"
)

foreach(data_path ${ESPEAK_DATA_SEARCH_PATHS})
  if(EXISTS "${data_path}")
    message("Copying espeak-ng-data from ${data_path} for test_piper")
    file(COPY "${data_path}" DESTINATION "${TARGET_DIR}")
    break()
  endif()
endforeach()