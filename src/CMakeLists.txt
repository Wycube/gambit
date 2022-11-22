# Frontend and emulator library
add_subdirectory(common)
add_subdirectory(emulator/core)
add_subdirectory(frontend)

# Unit Tests
if(BUILD_TESTS)
	add_subdirectory(tests)
endif()