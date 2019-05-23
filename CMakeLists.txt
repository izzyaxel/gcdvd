cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
if(WIN32 OR WIN64)
	add_definitions(-DWINDOWS)
elseif(UNIX)
	add_definitions(-DLINUX)
elseif(APPLE)
	add_definitions(-DOSX)
endif()

project(gcdvd)
set(CMAKE_CXX_STANDARD 17)
add_library(${PROJECT_NAME} STATIC
		src/dvd.cc src/dvd.hh)
set_target_properties(${PROJECT_NAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs)
