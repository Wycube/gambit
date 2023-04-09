#include "Drawing.hpp"
#include "Types.hpp"
#include "common/Bits.hpp"
#include "common/Log.hpp"


namespace emu {

void Background::reset() {
    priority = 0;
    char_base_block = 0;
    mosaic = 0;
    color_mode = 0;
    scr_base_block = 0;
    disp_overflow = 0;
    screen_size = 0;
    unused = 0;
    h_offset = 0;
    v_offset = 0;
    
    reference_x = 0;
    reference_y = 0;
    param_a = 0x100;
    param_b = 0;
    param_c = 0;
    param_d = 0x100;
}

void Background::write(u32 address, u8 value) {
    if(address & 1) {
        scr_base_block = value & 0x1F;
        disp_overflow = (value >> 5) & 1;
        screen_size = (value >> 6) & 3;
    } else {
        priority = value & 3;
        char_base_block = (value >> 2) & 3;
        unused = (value >> 4) & 3;
        mosaic = (value >> 6) & 1;
        color_mode = (value >> 7) & 1;
    }
}

auto Background::read(u32 address, bool regular) -> u8 {
    if(address & 1) {
        return scr_base_block | ((!regular && disp_overflow) << 5) | (screen_size << 6);
    } else {
        return priority | (char_base_block << 2) | (unused << 4) | (mosaic << 6) | (color_mode << 7);
    }
}

auto Background::getTextPixel(int x, int y, const u8 *vram, const PPUState &state) -> u8 {
    const u16 map_width = 32 << (screen_size & 1);
    const u16 map_height = 32 << (screen_size >> 1);
    const u8 tile_width = 4 << color_mode; //color_mode ? 8 : 4;

    if(mosaic) {
        int mosaic_w = (bits::get<0, 4>(state.mosaic) + 1);
        int mosaic_h = (bits::get<4, 4>(state.mosaic) + 1);
        x = x / mosaic_w * mosaic_w;
        y = y / mosaic_h * mosaic_h;
    }

    x += h_offset;
    y += v_offset;
    x %= map_width * 8;
    y %= map_height * 8;


    const u8 tile_x = x >> 3;
    const u8 tile_y = y >> 3;
    const u8 screen_block = (tile_x >> 5) + (tile_y >> 5) * (map_width >> 5);

    const u32 tile_index = screen_block * 1024 + (tile_x & 0x1F) + ((tile_y & 0x1F) << 5);
    const u32 map_data_address = 0x800 * scr_base_block + tile_index * 2;
    const u16 tile_entry = (vram[map_data_address + 1] << 8) | vram[map_data_address];
    const bool mirror_x = bits::get_bit<10>(tile_entry);
    const bool mirror_y = bits::get_bit<11>(tile_entry);
    u8 tile_pixel_x = x & 7;
    u8 tile_pixel_y = y & 7;

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

auto Background::getAffinePixel(int x, int y, const u8 *vram) -> u8 {
    const int map_size = (16 << screen_size) * 8;
    getAffineCoords(x, y);

    if(x < 0 || x >= map_size || y < 0 || y >= map_size) {
        if(disp_overflow) {
            x %= map_size;
            y %= map_size;

            if(x < 0) x = map_size + x;
            if(y < 0) y = map_size + y;
        } else {
            return 0;
        }
    }

    const u8 tile_x = x >> 3;
    const u8 tile_y = y >> 3;
    const u8 tile_pixel_x = x & 7;
    const u8 tile_pixel_y = y & 7;

    const u32 tile_index = tile_x + tile_y * (map_size / 8);
    const u8 tile_entry = vram[0x800 * scr_base_block + tile_index];
    const u8 palette_index = vram[0x4000 * char_base_block + tile_entry * 64 + tile_pixel_x + tile_pixel_y * 8];

    return palette_index;
}

auto Background::getBitmapPixelMode3(int x, int y, const u8 *vram) -> u16 {
    getAffineCoords(x, y);
    const u32 index = x + y * 240;

    if(x < 0 || x >= 240 || y < 0 || y >= 160) {
        return 0;
    }
    
    return (vram[index * 2 + 1] << 8) | vram[index * 2];
}

auto Background::getBitmapPixelMode4(int x, int y, const u8 *vram, const u8 *palette, bool frame_1) -> u16 {
    getAffineCoords(x, y);
    const u32 index = x + y * 240;
    const u32 data_start = frame_1 ? 0xA000 : 0;
    const u8 color_index = vram[data_start + index];

    if(x < 0 || x >= 240 || y < 0 || y >= 160) {
        return 0;
    }
    
    return (palette[color_index * 2 + 1] << 8) | palette[color_index * 2];
}

auto Background::getBitmapPixelMode5(int x, int y, const u8 *vram, bool frame_1) -> u16 {
    getAffineCoords(x, y);

    if(x < 0 || x >= 160 || y < 0 || y >= 128) {
        return 0;
    }

    u32 index = x + y * 160;
    u32 data_start = frame_1 ? 0xA000 : 0;
    return vram[data_start + index];
}

void Background::resetInternalRegs() {
    internal_x = reference_x;
    internal_y = reference_y;
}

void Background::incrementInternalRegs() {
    internal_x += param_b;
    internal_y += param_d;
}

void Background::getAffineCoords(int &x, int &y) {
    int new_x = (param_a * (x << 8) >> 8) + internal_x;
    int new_y = (param_c * (x << 8) >> 8) + internal_y;

    x = new_x >> 8;
    y = new_y >> 8;
}

auto Object::getScreenX(int local_x) const -> int {
    return (x + local_x) & 0x1FF;
}

auto Object::getObjectPixel(int local_x, int local_y, const PPUState &state) const -> u8 {
    const bool color_mode = bits::get_bit<5>(state.oam[index * 8 + 1]);
    const bool mirror_x = bits::get_bit<4>(state.oam[index * 8 + 3]);
    const bool mirror_y = bits::get_bit<5>(state.oam[index * 8 + 3]);
    const u8 tile_width = color_mode ? 8 : 4;
    
    if(affine) {
        getAffineCoords(local_x, local_y, state);

        if(local_x < 0 || local_x >= width || local_y < 0 || local_y >= height) {
            return 0;
        }
    }

    if(!affine && mirror_x) local_x = width - local_x - 1;
    if(!affine && mirror_y) local_y = height - local_y - 1;

    u8 tile_x = local_x / 8;
    u8 tile_y = local_y / 8;
    local_x %= 8;
    local_y %= 8;

    if(color_mode) {
        tile_x <<= 1;
    }

    u32 tile_address = (((state.oam[index * 8 + 5] & 3) << 8) | state.oam[index * 8 + 4]);
    if(bits::get_bit<6>(state.dispcnt)) {
        tile_address += tile_x + tile_y * (width >> (color_mode ? 2 : 3));
    } else {
        tile_address += (tile_x & (color_mode ? ~1 : ~0)) + tile_y * 32;
    }
    tile_address &= 0x3FF;

    u8 palette_index = state.vram[0x10000 + tile_address * 32 + (local_x >> !color_mode) + local_y * tile_width];

    if(!color_mode) {
        palette_index = (palette_index >> (local_x & 1) * 4) & 0xF;

        if(palette_index == 0) {
            return 0;
        }

        palette_index = palette_index + bits::get<4, 4>(state.oam[index * 8 + 5]) * 16;
    }

    return palette_index;
}

void Object::getAffineCoords(int &local_x, int &local_y, const PPUState &state) const {
    u32 param_address = param_select * 4;
    s16 param_a = (state.oam[(param_address + 0) * 8 + 7] << 8) | state.oam[(param_address + 0) * 8 + 6];
    s16 param_b = (state.oam[(param_address + 1) * 8 + 7] << 8) | state.oam[(param_address + 1) * 8 + 6];
    s16 param_c = (state.oam[(param_address + 2) * 8 + 7] << 8) | state.oam[(param_address + 2) * 8 + 6];
    s16 param_d = (state.oam[(param_address + 3) * 8 + 7] << 8) | state.oam[(param_address + 3) * 8 + 6];

    local_x -= double_size ? width : width / 2;
    local_y -= double_size ? height : height / 2;
    local_x <<= 8;
    local_y <<= 8;

    int new_x = ((param_a * local_x + param_b * local_y) >> 16) + width / 2;
    int new_y = ((param_c * local_x + param_d * local_y) >> 16) + height / 2;

    local_x = new_x;
    local_y = new_y;
}

void Window::reset() {
    winh[0] = 0;
    winh[1] = 0;
    winv[0] = 0;
    winv[1] = 0;
    winin = 0;
    winout = 0;
}

//TODO: Window has some weird behavior to implement
auto Window::insideWindow(int x, int y, int window) -> bool {
    const u8 left = winh[window] >> 8;
    const u8 right = winh[window] & 0xFF;
    const u8 top = winv[window] >> 8;
    const u8 bottom = winv[window] & 0xFF;

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