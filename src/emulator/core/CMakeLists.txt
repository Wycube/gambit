if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wpedantic -Wextra)
endif()

# Get all source files
file(GLOB all_src 
    "*.cpp"
    "cpu/*.cpp"
    "cpu/arm/*.cpp"
    "cpu/thumb/*.cpp"
    "mem/*.cpp"
    "mem/save/*.cpp"
    "mem/gpio/*.cpp"
    "debug/*.cpp"
    "ppu/*.cpp"
    "apu/*.cpp"
    "apu/channels/*.cpp"
)
add_library(gba-lib ${all_src})
target_link_libraries(gba-lib fmt common_logging)
set_property(TARGET gba-lib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)