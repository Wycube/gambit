if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory.")
endif()

cmake_minimum_required(VERSION 3.10)

project(gba-emu VERSION 0.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

option(BUILD_TESTS "Build tests" OFF)

find_package(Git)
if(GIT_FOUND)
	# Git describe information
	execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} describe --tags --always --long --dirty
	OUTPUT_VARIABLE GIT_DESCRIBE 
	OUTPUT_STRIP_TRAILING_WHITESPACE)

	# Remove trailing information from output
	string(REGEX REPLACE "(-0)?-g([0-9]|[a-f])+" "" GIT_DESCRIBE "${GIT_DESCRIBE}")

	# Git rev-parse current commit hash
	execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
	OUTPUT_VARIABLE GIT_COMMIT 
	OUTPUT_STRIP_TRAILING_WHITESPACE)

	# Git branch current branch name
	execute_process(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} branch --show-current
	OUTPUT_VARIABLE GIT_BRANCH 
	OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	message(FATAL_ERROR "Git was not found!")
endif()

include_directories(${PROJECT_SOURCE_DIR}/lib/fmt/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/glfw/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/imgui-docking)
include_directories(${PROJECT_SOURCE_DIR}/lib/glad/include)
include_directories(${PROJECT_SOURCE_DIR}/lib/miniaudio/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_subdirectory(${PROJECT_SOURCE_DIR}/src)