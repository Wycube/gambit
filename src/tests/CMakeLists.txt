include_directories(${PROJECT_SOURCE_DIR}/lib/lest-1.35.1/include)
add_executable(tests testmain.cpp)
target_link_libraries(tests gba-lib)