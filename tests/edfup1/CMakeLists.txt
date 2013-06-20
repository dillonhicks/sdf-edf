# Required project namespacing because CMake uses it to set a whole
# host of auto-magic variables that are convenient for larger
# projects. Typically these are in the form <PROJECT-NAME>_<VAR-NAME>.
#
# Note that there is also PROJECT_NAME which will store the value sent
# to project().
#
project(edfup1)

# Required version check, or CMake will throw a tantrum.
cmake_minimum_required(VERSION 2.6)

# Setting the CMake module path to specify the 
# directory into which CMake should look to 
# find extra modules, I.E. FindLibXml2.cmake
#
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(EDF_WORKSPACE_DIR $ENV{KUSPROOT}/workspace/sdf_edf)

set(LIBHGS_DIR ${EDF_WORKSPACE_DIR}/libhgs)

set(KUSP_HGS_DIR $ENV{KUSPROOT}/subsystems/hgs)

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
  ${CMAKE_CURRENT_SOURCE_DIR}/config
  ${CMAKE_CURRENT_SOURCE_DIR}/xhashconf
  ${LIBXML2_INCLUDE_DIRS}
  ${KUSP_HGS_DIR}/include
  ${EDF_WORKSPACE_DIR}/include  
  )


# Flags that CMAKE will pass to GCC not related to source files,
# includes, and libraries.
#
set(CMAKE_C_FLAGS "-DCONFIG_DSUI -DHAVE_KUSP -Wall ${LIBXML2_DEFINITIONS} -DHAVE_GSL")

add_library(sdf_edf SHARED 
  ${LIBHGS_DIR}/sdf_edf.c
  )


# Create an executable target with the same name as the projects from
# the files that follow it.
add_executable(${PROJECT_NAME}
  ${PROJECT_NAME}.c 
  ${PROJECT_NAME}_dsui.c
  ${CMAKE_CURRENT_SOURCE_DIR}/config/test_config.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/xhashconf/xhashconf.c 
  )


# Libs for GCC -l
target_link_libraries(${PROJECT_NAME}
  pthread 
  dsui
  kusp
  ccsm
  hgs
  sdf_edf
  ${LIBXML2_LIBRARIES}
  m
  gsl
  gslcblas
  )

# Make a list of files from the globbing expressions relative to the
# source directory and store that list in CONFIG_FILES.
#
file(GLOB CONFIG_FILES 
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  *cpu/*.dski
  *cpu/*.pipes
  *cpu/*.hgs
  *cpu/*.xml
  )

  
# Copy all of the configuration files into the binary directory.
#
foreach(CF ${CONFIG_FILES})

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CF} 
    ${CMAKE_CURRENT_BINARY_DIR}/${CF} COPYONLY)

endforeach(CF)
