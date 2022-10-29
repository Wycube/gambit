#include "OGLVideoDevice.hpp"
#include "common/Log.hpp"


OGLVideoDevice::OGLVideoDevice() {
    new_frame = false;

    createTexture();
    clear(0);
    updateTexture(m_present_framebuffer);

    for(size_t i = 0; i < m_frame_times.capacity(); i++) {
        m_frame_times.push(0);
    }
}

OGLVideoDevice::~OGLVideoDevice() {
    delete[] m_present_framebuffer;

    if(m_texture_id != 0) {
        glDeleteTextures(1, &m_texture_id);
    }
}

void OGLVideoDevice::clear(u32 color) {
    std::lock_guard lock(m_update_mutex);

    for(size_t i = 0; i < sizeof(m_internal_framebuffer) / 4; i++) {
        m_internal_framebuffer[i] = color;
        m_present_framebuffer[i] = color;
    }
}

void OGLVideoDevice::setPixel(int x, int y, u32 color) {
    std::lock_guard lock(m_update_mutex);

    m_internal_framebuffer[x + y * 240] = color;
}

void OGLVideoDevice::setLine(int y, u32 *colors) {
    std::lock_guard lock(m_update_mutex);
    
    std::memcpy(&m_internal_framebuffer[y * 240], colors, 240 * sizeof(u32));
}

void OGLVideoDevice::presentFrame() {
    std::lock_guard lock(m_update_mutex);

    auto now = std::chrono::steady_clock::now();
    auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start);
    m_start = now;

    m_frame_times.push(frame_time.count() / 1000.0f);

    std::memcpy(m_present_framebuffer, m_internal_framebuffer, sizeof(m_internal_framebuffer));
    new_frame = true;
}

auto OGLVideoDevice::getTextureID() -> GLuint {
    std::lock_guard lock(m_update_mutex);
    
    if(new_frame) {
        updateTexture(m_present_framebuffer);
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

    #ifndef NDEBUG
    glObjectLabel(GL_TEXTURE, m_texture_id, -1, "OGLVideoDevice Frame Texture");
    #endif

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OGLVideoDevice::updateTexture(u32 *pixels) {
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 240, 160, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}