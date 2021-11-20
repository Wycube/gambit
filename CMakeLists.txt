if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory.")
endif()

cmake_minimum_required(VERSION 3.10)

project(gba-emu)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

option(BUILD_TESTS "Build tests" OFF)


# Version information
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)

find_package(Git)
if(GIT_FOUND)
	execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} describe --always --long
	OUTPUT_VARIABLE GIT_DESCRIBE 
	OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	message(FATAL_ERROR "Git was not found")
endif()


# lest (Unit Testing)
include_directories(${PROJECT_SOURCE_DIR}/lib/lest-1.35.1/include)

# Add frontend and emulator library
include_directories(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/common)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/core)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/frontend)