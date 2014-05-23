cmake_minimum_required(VERSION 2.6)

# define a macro that helps defining an option
macro(set_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()

set_option(CMAKE_BUILD_TYPE Release STRING "Debug: -g -O0, Release: -O2")

project(opengl-tests)

# Configure OS versions with some variables
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${PROJECT_SOURCE_DIR}/extlibs/headers)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(OS_WINDOWS 1)
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${PROJECT_SOURCE_DIR}/extlibs/libs-win64)
  add_definitions(-DGLEW_STATIC)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS_LINUX 1)
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${PROJECT_SOURCE_DIR}/extlibs/libs-linux)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(OS_OSX 1)
  set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${PROJECT_SOURCE_DIR}/extlibs/libs-osx)
else()
  message("FATAL: Unsupported OS")
  return()
endif()

# set some options
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CPP_FLAGS_DEBUG} -g -Wall -Wextra -O0 -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CPP_FLAGS_RELEASE} -Wall -Wextra -O2 -std=c++11")

set(SRC ${PROJECT_SOURCE_DIR}/src)

# durectories
include_directories(${SRC})
set(EXECUTABLE_OUTPUT_PATH bin)

# modules
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# find files
file(GLOB_RECURSE source_files ${SRC}/*)

# Testing executable
set(EXEC opengl-test)
add_executable(
  ${EXEC}
  ${source_files}
)

# Finding libs
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIR} )

# libs
target_link_libraries(${EXEC} ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})

# tests
#enable_testing()
#add_test(tcatimg ${PROJECT_SOURCE_DIR}/tests/tcatimg.sh)

# Doc

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)
