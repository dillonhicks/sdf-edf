# Required project namespacing because CMake uses it to set a whole
# host of auto-magic variables that are convenient for larger
# projects. Typically these are in the form <PROJECT-NAME>_<VAR-NAME>.
#
# Note that there is also PROJECT_NAME which will store the value sent
# to project().
#
project(kuspconf)

# Required version check, or CMake will throw a tantrum.
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Since libxml2 is a little strange we have to the super special
# finder library (./cmake/FindLibXml2.cmake). This will set a few
# variables (defined in the source file) that makes our life a little
# easier. These variables are in the form LIBXML2_<NAME>. Also it may
# be useful to test the result variable LIBXML2_FOUND, but GCC will
# yell at us anyway.
include(FindLibXml2)

# GCC Include directories for -I
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR}/uthash
  ${LIBXML2_INCLUDE_DIRS}
  )

# Flags that CMAKE will pass to GCC not related to source files,
# includes, and libraries.
#
set(CMAKE_C_FLAGS "-Wall ${LIBXML2_DEFINITIONS}")

add_library(xhashconf SHARED
  xhashconf.c
  )


# Create an executable target with the same name as the projects from
# the files that follow it.
add_executable(${PROJECT_NAME}
  main.c
  )

# Libs for GCC -l
target_link_libraries(${PROJECT_NAME}
  ${LIBXML2_LIBRARIES}
  xhashconf
  )

