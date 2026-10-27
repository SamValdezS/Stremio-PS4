// Installed add-ons, public catalogs and confirmed account actions.
#pragma once

#include "../core/models.h"
#include "screen.h"
#include "tab_bar.h"

#include <memory>
#include <string>
#include <vector>

class AddonsScreen : public Screen {
public:
        AddonsScreen(Fonts &fonts, Navigator &nav);
        void update(double dt, uint32_t pressed) override;
        void draw(Renderer &r) override;

private:
        struct CatalogState {
                std::vector<Addon> items;
                bool loading = false;
                bool loaded = false;
                bool failed = false;
        };

        const std::vector<Addon> &current_addons() const;
        void set_filter(int filter);
        void load_catalog(bool force = false);
        void draw_filters(Renderer &r);
        bool is_installed(const Addon &addon) const;
        void activate_addon();
        void execute_change();
        void draw_action(Renderer &r);
        void draw_confirmation(Renderer &r);

        std::shared_ptr<int> alive_ = std::make_shared<int>(0);
        ui::Toast toast_;
        bool operation_pending_ = false;
        bool confirming_ = false;
        bool confirm_ok_ = false;
        bool confirm_install_ = false;
        int confirm_auth_rev_ = -1;
        Addon confirmed_addon_;

        int filter_ = 0;
        bool on_filters_ = false;
        std::shared_ptr<CatalogState> catalogs_[2] = {
                std::make_shared<CatalogState>(),
                std::make_shared<CatalogState>()
        };

        Fonts &fonts_;
        Navigator &navigator_;
        tabs::TabBar bar_{ tabs::ADDONS };
        int selected_ = 0;
        float scroll_ = 0;
        bool on_bar_ = false;
        std::vector<float> focus_;
};
