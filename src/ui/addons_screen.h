// Read-only view of the add-ons currently loaded by the client.
#pragma once

#include "screen.h"
#include "tab_bar.h"

#include <vector>

class AddonsScreen : public Screen {
public:
        AddonsScreen(Fonts &fonts, Navigator &nav);
        void update(double dt, uint32_t pressed) override;
        void draw(Renderer &r) override;

private:
        Fonts &fonts_;
        Navigator &navigator_;
        tabs::TabBar bar_{ tabs::ADDONS };
        int selected_ = 0;
        float scroll_ = 0;
        bool on_bar_ = false;
        std::vector<float> focus_;
};
