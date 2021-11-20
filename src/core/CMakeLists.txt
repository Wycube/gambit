# Get all source files
file(GLOB all_src "*.cpp" "cpu/*.cpp" "cpu/arm/*.cpp" "cpu/thumb/*.cpp")
add_library(gba-lib ${all_src})