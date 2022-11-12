#pragma once


namespace ui {

class Window {
public:

    void update() {
        if(!m_active) {
            return;
        }

        draw();
    }

    auto getActive() -> bool* {
        return &m_active;
    }

    auto isActive() -> bool {
        return m_active;
    }

    auto getName() -> const char* {
        return m_name;
    }

protected:

    bool m_active;
    const char *m_name;

    virtual void draw() = 0;
};

} //namespace ui