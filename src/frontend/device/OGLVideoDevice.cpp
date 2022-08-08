#include "OGLVideoDevice.hpp"


OGLVideoDevice::OGLVideoDevice() {
    createTexture();
    clear(0);
    presentFrame();
}

OGLVideoDevice::~OGLVideoDevice() {
    if(m_texture_id != 0) {
        glDeleteTextures(1, &m_texture_id);
    }
}

void OGLVideoDevice::clear(u32 color) {
    for(int i = 0; i < sizeof(m_internal_framebuffer) / 4; i++) {
        m_internal_framebuffer[i] = color;
    }
}

void OGLVideoDevice::setPixel(int x, int y, u32 color) {
    m_internal_framebuffer[x + y * 240] = color;
}

void OGLVideoDevice::presentFrame() {
    std::lock_guard lock(m_update_mutex);
    new_frame = true;
}

auto OGLVideoDevice::getTextureID() -> GLuint {
    std::lock_guard lock(m_update_mutex);
    if(new_frame) {
        updateTexture(m_internal_framebuffer);
        new_frame = false;
    }

    return m_texture_id;
}

void OGLVideoDevice::createTexture() {
    glGenTextures(1, &m_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    #if _DEBUG
    glObjectLabel(GL_TEXTURE, m_texture_id, -1, "OGLVideoDevice Frame Texture");
    #endif

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OGLVideoDevice::updateTexture(u32 *pixels) {
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 240, 160, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}