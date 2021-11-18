# Get all source files
file(GLOB all_src "core/*.cpp" "core/cpu/*.cpp" "core/cpu/arm/*.cpp" "core/cpu/thumb/*.cpp")
add_library(gba ${all_src})