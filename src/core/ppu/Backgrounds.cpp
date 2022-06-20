#include "Backgrounds.hpp"
#include "common/Bits.hpp"

#include <cstdio>


namespace emu {

auto Background0::getPixelColor(int x, int y, u8 *vram, u8 *palette) -> u32 {
    u32 char_data_base = 0x4000 * bits::get<2, 2>(bg0cnt); //Offset starting from 0x06000000 VRAM
    u32 map_data_base = 0x800 * bits::get<8, 5>(bg0cnt); //Offset starting from 0x06000000 VRAM
    bool color_mode = bits::get<7, 1>(bg0cnt); //1 is 256 color or 8bpp, 0 is 16 color or 4bpp
    int map_width = 32 << bits::get<14, 1>(bg0cnt);
    int map_height = 32 << bits::get<13, 1>(bg0cnt);

    //Apply offset
    x += bg0hofs;
    x %= map_width * 8;
    y += bg0vofs;
    y %= map_height * 8;

    int tile_x = x / 8;
    int tile_y = y / 8;
    int screen_block = (tile_x / 32) + (tile_y / 32) * (map_width / 32);
    tile_x %= 32;
    tile_y %= 32;
    int tile_pixel_x = (x % 8) / (color_mode ? 1 : 2);
    int tile_pixel_y = y % 8;

    u32 map_data_address = map_data_base + screen_block * 0x800 + tile_x * 2 + tile_y * map_width;
    u16 tile_entry = (vram[map_data_address + 1] << 8) | vram[map_data_address];
    bool flip_x = bits::get<10, 1>(tile_entry);
    bool flip_y = bits::get<11, 1>(tile_entry);

    if(flip_x) tile_pixel_x = (color_mode ? 7 : 3) - tile_pixel_x;
    if(flip_y) tile_pixel_y = 7 - tile_pixel_y;

    u8 palette_selected = bits::get<12, 4>(tile_entry);
    u8 palette_index = vram[char_data_base + bits::get<0, 10>(tile_entry) * (color_mode ? 64 : 32) + tile_pixel_x + tile_pixel_y * (color_mode ? 8 : 4)];
    if(!color_mode) {
        palette_index = (palette_index >> (x & 1) * 4) & 0xF;
    }
    if(!color_mode) palette_index = palette_index + palette_selected * 16;
    u16 color_16 = (palette[palette_index * 2 + 1] << 8) | palette[palette_index * 2];
    u8 red = bits::get<0, 5>(color_16) * 8;
    u8 green = bits::get<5, 5>(color_16) * 8;
    u8 blue = bits::get<10, 5>(color_16) * 8;

    return (red << 24) | (green << 16) | (blue << 8) | 0xFF;
}

} //namespace emu