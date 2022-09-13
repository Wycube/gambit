#include "Drawing.hpp"
#include "common/Bits.hpp"


namespace emu {

void Background::write(u32 address, u8 value) {
    if(address & 1) {
        scr_base_block = value & 0x1F;
        disp_overflow = (value >> 5) & 1;
        screen_size = (value >> 6) & 3;
    } else {
        priority = value & 3;
        char_base_block = (value >> 2) & 3;
        mosaic = (value >> 6) & 1;
        color_mode = (value >> 7) & 1;
    }
}

auto Background::read(u32 address) -> u8 {
    if(address & 1) {
        return scr_base_block | (disp_overflow << 5) | (screen_size << 6);
    } else {
        return priority | (char_base_block << 2) | (mosaic << 6) | (color_mode << 7);
    }
}

auto Background::getTextPixel(int x, int y, const u8 *vram) -> u8 {
    const int map_width = 32 << (screen_size & 1);
    const int map_height = 32 << (screen_size >> 1);
    const int tile_width = color_mode ? 8 : 4;

    //Apply offset
    x += h_offset;
    x %= map_width * 8;
    y += v_offset;
    y %= map_height * 8;

    const int tile_x = x / 8;
    const int tile_y = y / 8;
    const int screen_block = (tile_x / 32) + (tile_y / 32) * (map_width / 32);

    const u32 tile_index = screen_block * 1024 + (tile_x % 32) + (tile_y % 32) * 32;
    const u32 map_data_address = 0x800 * scr_base_block + tile_index * 2;
    const u16 tile_entry = (vram[map_data_address + 1] << 8) | vram[map_data_address];
    const bool mirror_x = bits::get_bit<10>(tile_entry);
    const bool mirror_y = bits::get_bit<11>(tile_entry);
    int tile_pixel_x = x % 8;
    int tile_pixel_y = y % 8;

    if(mirror_x) tile_pixel_x = 7 - tile_pixel_x;
    if(mirror_y) tile_pixel_y = 7 - tile_pixel_y;
    tile_pixel_x >>= !color_mode;

    const u8 palette_selected = bits::get<12, 4>(tile_entry);
    u8 palette_index = vram[0x4000 * char_base_block + bits::get<0, 10>(tile_entry) * tile_width * 8 + tile_pixel_x + tile_pixel_y * tile_width];
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
    const int map_size = 16 << screen_size;
    const int x2 = _a * (float)x + _b * (float)y + _x;
    const int y2 = _c * (float)x + _d * (float)y + _y;

    //TODO: Handle the setting in the background control register
    if(x2 < 0 || x2 >= map_size * 8 || y2 < 0 || y2 >= map_size * 8) {
        return 0;
    }

    const int tile_x = x2 / 8;
    const int tile_y = y2 / 8;
    const int tile_pixel_x = x2 % 8;
    const int tile_pixel_y = y2 % 8;

    const u32 tile_index = tile_x + tile_y * map_size;
    const u8 tile_entry = vram[0x800 * scr_base_block + tile_index];
    const u8 palette_index = vram[0x4000 * char_base_block + tile_entry * 64 + tile_pixel_x + tile_pixel_y * 8];

    return palette_index;
}

auto Background::getBitmapPixelMode3(int x, int y, const u8 *vram) -> u16 {
    const u32 index = x + y * 240;
    
    return (vram[index * 2 + 1] << 8) | vram[index * 2];
}

auto Background::getBitmapPixelMode4(int x, int y, const u8 *vram, const u8 *palette, bool frame_1) -> u16 {
    const u32 index = x + y * 240;
    const u32 data_start = frame_1 ? 0xA000 : 0;
    const u8 color_index = vram[data_start + index];
    
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
    const u8 left = win0h >> 8;
    const u8 right = win0h & 0xFF;
    const u8 top = win0v >> 8;
    const u8 bottom = win0v & 0xFF;

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
    const u8 left = win1h >> 8;
    const u8 right = win1h & 0xFF;
    const u8 top = win1v >> 8;
    const u8 bottom = win1v & 0xFF;

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