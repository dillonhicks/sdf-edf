
cmake_minimum_required(VERSION 2.6)

project( UNIFIED_HELLOWORLD )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(CMAKE_C_FLAGS "-DCONFIG_DSUI -Wall")

execute_process( COMMAND dsui-header --prefix helloworld WORKING_DIRECTORY
	${CMAKE_CURRENT_BINARY_DIR} )

add_executable( helloworld ${CMAKE_CURRENT_SOURCE_DIR}/helloworld.c
	${CMAKE_CURRENT_BINARY_DIR}/helloworld_dsui.c )

target_link_libraries(helloworld pthread dsui kusp ccsm gsched m)


