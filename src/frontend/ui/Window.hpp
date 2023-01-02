#pragma once


namespace ui {

class Window {
public:

    void update() {
        if(!active) {
            return;
        }

        draw();
    }

    auto getActive() -> bool* {
        return &active;
    }

    auto isActive() -> bool {
        return active;
    }

    auto getName() -> const char* {
        return name;
    }

protected:

    bool active;
    const char *name;

    virtual void draw() = 0;
};

} //namespace ui