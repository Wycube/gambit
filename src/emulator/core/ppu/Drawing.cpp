#include "Drawing.hpp"
#include "common/Bits.hpp"


namespace emu {

auto Background::getTextPixel(int x, int y, const u8 *vram) -> u8 {
    u32 char_data_base = 0x4000 * bits::get<2, 2>(control); //Offset starting from 0x06000000 VRAM
    u32 map_data_base = 0x800 * bits::get<8, 5>(control); //Offset starting from 0x06000000 VRAM
    bool color_mode = bits::get_bit<7>(control); //1 is 256 color or 8bpp, 0 is 16 color or 4bpp
    int map_width = 32 << bits::get_bit<14>(control);
    int map_height = 32 << bits::get_bit<15>(control);
    int tile_width = color_mode ? 8 : 4;

    //Apply offset
    x += h_offset;
    x %= map_width * 8;
    y += v_offset;
    y %= map_height * 8;

    int tile_x = x / 8;
    int tile_y = y / 8;
    int screen_block = (tile_x / 32) + (tile_y / 32) * (map_width / 32);

    u32 tile_index = screen_block * 1024 + (tile_x % 32) + (tile_y % 32) * 32;
    u32 map_data_address = map_data_base + tile_index * 2;
    u16 tile_entry = (vram[map_data_address + 1] << 8) | vram[map_data_address];
    bool mirror_x = bits::get_bit<10>(tile_entry);
    bool mirror_y = bits::get_bit<11>(tile_entry);
    int tile_pixel_x = x % 8;
    int tile_pixel_y = y % 8;

    if(mirror_x) tile_pixel_x = 7 - tile_pixel_x;
    if(mirror_y) tile_pixel_y = 7 - tile_pixel_y;
    tile_pixel_x >>= !color_mode;

    u8 palette_selected = bits::get<12, 4>(tile_entry);
    u8 palette_index = vram[char_data_base + bits::get<0, 10>(tile_entry) * tile_width * 8 + tile_pixel_x + tile_pixel_y * tile_width];
    if(!color_mode) {
        bool odd = (x & 1) ^ mirror_x;
        palette_index = (palette_index >> odd * 4) & 0xF;

        if(palette_index == 0) {
            return 0;
        }

        palette_index += palette_selected * 16;
    }

    return palette_index;
}

void Background::updateAffineParams() {
    _x = (bits::sign_extend<20, s32>(bits::get<8, 20>(reference_x)) + (float)bits::get<0, 8>(reference_x) / 256.0f);
    _y = (bits::sign_extend<20, s32>(bits::get<8, 20>(reference_y)) + (float)bits::get<0, 8>(reference_y) / 256.0f);

    _a = ((s8)bits::get<8, 8>(param_a) + (float)bits::get<0, 8>(param_a) / 256.0f);
    _b = ((s8)bits::get<8, 8>(param_b) + (float)bits::get<0, 8>(param_b) / 256.0f);
    _c = ((s8)bits::get<8, 8>(param_c) + (float)bits::get<0, 8>(param_c) / 256.0f);
    _d = ((s8)bits::get<8, 8>(param_d) + (float)bits::get<0, 8>(param_d) / 256.0f);
}

auto Background::getAffinePixel(int x, int y, const u8 *vram) -> u8 {
    u32 char_data_base = 0x4000 * bits::get<2, 2>(control); //Offset starting from 0x06000000 VRAM
    u32 map_data_base = 0x800 * bits::get<8, 5>(control); //Offset starting from 0x06000000 VRAM
    int map_size = 16 << bits::get<14, 2>(control);
    int x2 = _a * (float)x + _b * (float)y + _x;
    int y2 = _c * (float)x + _d * (float)y + _y;

    //TODO: Handle the setting in the background control register
    if(x2 < 0 || x2 >= map_size * 8 || y2 < 0 || y2 >= map_size * 8) {
        return 0;
    }

    int tile_x = x2 / 8;
    int tile_y = y2 / 8;
    int tile_pixel_x = x2 % 8;
    int tile_pixel_y = y2 % 8;

    u32 tile_index = tile_x + tile_y * map_size;
    u8 tile_entry = vram[map_data_base + tile_index];
    u8 palette_index = vram[char_data_base + tile_entry * 64 + tile_pixel_x + tile_pixel_y * 8];

    return palette_index;
}

auto Background::getBitmapPixelMode3(int x, int y, const u8 *vram) -> u16 {
    u32 index = x + y * 240;
    return (vram[index * 2 + 1] << 8) | vram[index * 2];
}

auto Background::getBitmapPixelMode4(int x, int y, const u8 *vram, const u8 *palette, bool frame_1) -> u16 {
    u32 index = x + y * 240;
    u32 data_start = frame_1 ? 0xA000 : 0;
    u8 color_index = vram[data_start + index];
    return (palette[color_index * 2 + 1] << 8) | palette[color_index * 2];
}

auto Background::getBitmapPixelMode5(int x, int y, const u8 *vram, bool frame_1) -> u16 {
    if(x > 160 || y > 128) {
        return 0;
    }

    u32 index = x + y * 160;
    u32 data_start = frame_1 ? 0xA000 : 0;
    return vram[data_start + index];
}

//TODO: Window has some weird behavior to implement
auto Window::insideWindow0(int x, int y) -> bool {
    u8 left = win0h >> 8;
    u8 right = win0h & 0xFF;
    u8 top = win0v >> 8;
    u8 bottom = win0v & 0xFF;

    bool in_horizontal = x >= left && x < right;
    bool in_vertical = y >= top && y < bottom;

    if(left > right) {
        in_horizontal = !(x < left && x >= right);
    }

    if(top > bottom) {
        in_vertical = !(y < top && y >= bottom);
    }

    return in_horizontal && in_vertical;
}

auto Window::insideWindow1(int x, int y) -> bool {
    u8 left = win1h >> 8;
    u8 right = win1h & 0xFF;
    u8 top = win1v >> 8;
    u8 bottom = win1v & 0xFF;

    bool in_horizontal = x >= left && x < right;
    bool in_vertical = y >= top && y < bottom;

    if(left > right) {
        in_horizontal = !(x < left && x >= right);
    }

    if(top > bottom) {
        in_vertical = !(y < top && y >= bottom);
    }

    return in_horizontal && in_vertical;
}

} //namespace emu