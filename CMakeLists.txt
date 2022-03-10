include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)

#############################################################
# MAX EXTERNAL
#############################################################

# the below is not working on Mac for me so finding and specifying 
# header and library explicitly
#find_package(Csound)

if(APPLE)
    find_path(CSOUND_HEADER_PATH csound.h)
    find_library(CSOUND_LIBRARY CsoundLib64)
else()
    find_path(CSOUND_HEADER_PATH csound/csound.h)
    find_library(CSOUND_LIBRARY csound64.lib)
endif()

if(CSOUND_HEADER_PATH)
    MESSAGE("found csound header path: ${CSOUND_HEADER_PATH}")
else()
    MESSAGE(FATAL_ERROR "could not find csound header path")
endif()
    
if(CSOUND_LIBRARY)
  MESSAGE("found csound library path: ${CSOUND_LIBRARY}")
else()
  MESSAGE(FATAL_ERROR "could not find csound library path")
endif()

include_directories( 
  "${MAX_SDK_INCLUDES}"
  "${MAX_SDK_MSP_INCLUDES}"
  "${MAX_SDK_JIT_INCLUDES}"
  "${CSOUND_HEADER_PATH}"
)

link_libraries(${CSOUND_LIBRARY})

file(GLOB PROJECT_SRC
   "*.h"
   "*.c"
   "*.cpp"
)
add_library( 
  ${PROJECT_NAME} 
  MODULE
  ${PROJECT_SRC}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
