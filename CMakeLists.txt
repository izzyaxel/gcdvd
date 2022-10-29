cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)
if(WIN32 OR WIN64)
	add_definitions(-DWINDOWS)
elseif(UNIX)
	add_definitions(-DLINUX)
elseif(APPLE)
	add_definitions(-DOSX)
endif()

project(gcdvd)
add_library(${PROJECT_NAME} STATIC
		src/dvd.cc src/dvd.hh src/main.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/libs)

project(dumptest)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(${PROJECT_NAME} src/main.cc)
add_dependencies(${PROJECT_NAME} gcdvd)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} gcdvd)
