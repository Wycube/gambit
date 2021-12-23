# Get all source files
file(GLOB all_src "*.cpp" "cpu/*.cpp" "cpu/arm/*.cpp" "cpu/thumb/*.cpp" "mem/*.cpp")
add_library(gba-lib ${all_src})
target_link_libraries(gba-lib fmt common_logging)