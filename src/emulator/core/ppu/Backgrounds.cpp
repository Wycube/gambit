#include "Backgrounds.hpp"
#include "common/Bits.hpp"


namespace emu {

auto TextBackground::getTextPixel(int x, int y, u8 *vram, u8 *palette) -> u32 {
    u32 char_data_base = 0x4000 * bits::get<2, 2>(bgcnt); //Offset starting from 0x06000000 VRAM
    u32 map_data_base = 0x800 * bits::get<8, 5>(bgcnt); //Offset starting from 0x06000000 VRAM
    bool color_mode = bits::get<7, 1>(bgcnt); //1 is 256 color or 8bpp, 0 is 16 color or 4bpp
    int map_width = 32 << bits::get<14, 1>(bgcnt);
    int map_height = 32 << bits::get<13, 1>(bgcnt);
    int tile_width = color_mode ? 8 : 4;

    //Apply offset
    x += bghofs;
    x %= map_width * 8;
    y += bgvofs;
    y %= map_height * 8;

    int tile_x = x / 8;
    int tile_y = y / 8;
    int screen_block = (tile_x / 32) + (tile_y / 32) * (map_width / 32);
    tile_x %= 32;
    tile_y %= 32;
    int tile_pixel_x = (x % 8) >> !color_mode;
    int tile_pixel_y = y % 8;

    u32 map_data_address = map_data_base + screen_block * 0x800 + tile_x * 2 + tile_y * map_width;
    u16 tile_entry = (vram[map_data_address + 1] << 8) | vram[map_data_address];
    bool flip_x = bits::get<10, 1>(tile_entry);
    bool flip_y = bits::get<11, 1>(tile_entry);

    if(flip_x) tile_pixel_x = (tile_width - 1) - tile_pixel_x;
    if(flip_y) tile_pixel_y = 7 - tile_pixel_y;

    u8 palette_selected = bits::get<12, 4>(tile_entry);
    u8 palette_index = vram[char_data_base + bits::get<0, 10>(tile_entry) * tile_width * 8 + tile_pixel_x + tile_pixel_y * tile_width];
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

auto BitmapBackground::getBitmapPixelMode3(int x, int y, u8 *vram) -> u32 {
    u32 index = x + y * 240;
    u16 color = (vram[index * 2 + 1] << 8) | vram[index * 2];

    u8 red = bits::get<0, 5>(color) * 8;
    u8 green = bits::get<5, 5>(color) * 8;
    u8 blue = bits::get<10, 5>(color) * 8;

    //Take the 16-bit, mode 3, color and turn it into a 24-bit color value
    return (red << 24) | (green << 16) | (blue << 8) | 0xFF;
}

auto BitmapBackground::getBitmapPixelMode4(int x, int y, u8 *vram, u8 *palette, bool frame_1) -> u32 {
    u32 index = x + y * 240;
    u32 data_start = frame_1 ? 0xA000 : 0;
    u8 color_index = vram[data_start + index];
    u16 color = (palette[color_index * 2 + 1] << 8) | palette[color_index * 2];

    u8 red = bits::get<0, 5>(color) * 8;
    u8 green = bits::get<5, 5>(color) * 8;
    u8 blue = bits::get<10, 5>(color) * 8;

    return (red << 24) | (green << 16) | (blue << 8) | 0xFF;
}

auto BitmapBackground::getBitmapPixelMode5(int x, int y, u8 *vram, bool frame_1) -> u32 {
    if(x > 160 || y > 128) {
        return 0;
    }

    u32 index = x + y * 160;
    u32 data_start = frame_1 ? 0xA000 : 0;
    u16 color = vram[data_start + index];

    u8 red = bits::get<0, 5>(color) * 8;
    u8 green = bits::get<5, 5>(color) * 8;
    u8 blue = bits::get<10, 5>(color) * 8;

    return (red << 24) | (green << 16) | (blue << 8) | 0xFF;
}

} //namespace emu