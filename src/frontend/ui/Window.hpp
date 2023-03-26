#pragma once


class Frontend;

namespace ui {

struct Window {
    bool active = false;

    virtual void draw(Frontend &frontend) = 0;
};

struct AboutDialog final : public Window {
    void open();
    void draw(Frontend &frontend) override;

private:

    bool open_popup = false;
};

struct FileDialog final : public Window {
    void open();
    void draw(Frontend &frontend) override;

private:

    bool open_popup = false;
};

struct MetricsWindow final : public Window {
    void draw(Frontend &frontend) override;

private:

    
};

} //namespace ui