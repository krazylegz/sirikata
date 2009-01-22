# - Find curl
# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIRS - where to find curl/curl.h, etc.
#  CURL_LIBRARIES    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.

FIND_PATH(CURL_INCLUDE_DIRS NAMES curl/curl.h PATHS ${CURL_ROOT} ${CURL_ROOT}/include DOC "The path containing the curl folder which in turn contains curl.h" NO_DEFAULT_PATH)
IF(NOT CURL_INCLUDE_DIRS) 
  FIND_PATH(CURL_INCLUDE_DIRS NAMES curl/curl.h DOC "The path containing the curl folder which in turn contains curl.h")
ENDIF(NOT CURL_INCLUDE_DIRS)

SET(CURL_FOUND FALSE)

IF(CURL_INCLUDE_DIRS)
  SET(CURL_LIBRARY_DIRS ${CURL_INCLUDE_DIRS})

  IF("${CURL_LIBRARY_DIRS}" MATCHES "/include$")
    # Regexp to stop  "/include" from seeping into the lib path.
    GET_FILENAME_COMPONENT(CURL_LIBRARY_DIRS ${CURL_LIBRARY_DIRS} PATH)
  ENDIF()

  IF(EXISTS "${CURL_LIBRARY_DIRS}/lib")
    SET(CURL_LIBRARY_DIRS ${CURL_LIBRARY_DIRS}/lib)
  ENDIF()

  IF(WIN32)
    FIND_LIBRARY(CURL_DEBUG_LIBRARY   NAMES curld curl_d libcurld libcurl_d
                 PATH_SUFFIXES "" Debug   PATHS ${CURL_LIBRARY_DIRS} NO_DEFAULT_PATH)
    FIND_LIBRARY(CURL_RELEASE_LIBRARY NAMES curl libcurl
                 PATH_SUFFIXES "" Release PATHS ${CURL_LIBRARY_DIRS} NO_DEFAULT_PATH)

    SET(CURL_LIBRARIES)
    IF(CURL_DEBUG_LIBRARY AND CURL_RELEASE_LIBRARY)
      SET(CURL_LIBRARIES debug ${CURL_DEBUG_LIBRARY} optimized ${CURL_RELEASE_LIBRARY})
    ELSEIF(CURL_DEBUG_LIBRARY)
      SET(CURL_LIBRARIES ${CURL_DEBUG_LIBRARY})
    ELSEIF(CURL_RELEASE_LIBRARY)
      SET(CURL_LIBRARIES ${CURL_RELEASE_LIBRARY})
    ENDIF(CURL_DEBUG_LIBRARY AND CURL_RELEASE_LIBRARY)
  ELSE()
    FIND_LIBRARY(CURL_LIBRARIES NAMES curl PATHS ${CURL_LIBRARY_DIRS} NO_DEFAULT_PATH)
  ENDIF()

  IF(CURL_LIBRARIES)
    SET(CURL_FOUND TRUE)
  ENDIF(CURL_LIBRARIES)
ENDIF(CURL_INCLUDE_DIRS)

IF(CURL_FOUND)
  IF(NOT CURL_FIND_QUIETLY)
    MESSAGE(STATUS "curl included: ${CURL_INCLUDE_DIRS}, libs: ${CURL_LIBRARIES}")
  ENDIF()
ELSE()
  IF(CURL_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "CURL not found")
  ENDIF()
ENDIF()
