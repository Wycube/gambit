#include "OGLVideoDevice.hpp"
#include "common/Log.hpp"


OGLVideoDevice::OGLVideoDevice() {
    LOG_DEBUG("Initializing OGLVideoDevice...");

    new_frame = false;
    createTexture();
    clear(0);
    updateTexture(present_framebuffer);
}

OGLVideoDevice::~OGLVideoDevice() {
    delete[] internal_framebuffer;
    delete[] present_framebuffer;

    if(texture_id != 0) {
        glDeleteTextures(1, &texture_id);
    }
}

void OGLVideoDevice::clear(u32 color) {
    std::lock_guard lock(update_mutex);

    for(size_t i = 0; i < (240 * 160); i++) {
        internal_framebuffer[i] = color;
        present_framebuffer[i] = color;
    }

    new_frame = true;
}

void OGLVideoDevice::setPixel(int x, int y, u32 color) {
    std::lock_guard lock(update_mutex);

    internal_framebuffer[x + y * 240] = color;
}

void OGLVideoDevice::setLine(int y, const u32 *colors) {
    std::lock_guard lock(update_mutex);
    
    std::memcpy(&internal_framebuffer[y * 240], colors, 240 * sizeof(u32));
}

void OGLVideoDevice::presentFrame() {
    std::lock_guard lock(update_mutex);
    std::memcpy(present_framebuffer, internal_framebuffer, 240 * 160 * sizeof(u32));
    new_frame = true;
}

auto OGLVideoDevice::getTextureID() -> GLuint {
    std::lock_guard lock(update_mutex);
    
    if(new_frame) {
        updateTexture(present_framebuffer);
        new_frame = false;
    }

    return texture_id;
}

void OGLVideoDevice::createTexture() {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    #ifndef NDEBUG
    glObjectLabel(GL_TEXTURE, texture_id, -1, "OGLVideoDevice Frame Texture");
    #endif

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OGLVideoDevice::updateTexture(u32 *pixels) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 240, 160, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}