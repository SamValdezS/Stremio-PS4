#include "addons_screen.h"

#include "../core/stremio.h"
#include "../platform/platform.h"
#include "components.h"
#include "i18n.h"
#include "theme.h"

#include <algorithm>
#include <string>

namespace {
using theme::S;
const float W = 1920;
const float MARGIN = 120;

float list_x() { return MARGIN; }
float list_y() { return S(170); }
float list_w() { return std::min(S(480), 560.0f); }
float row_h() { return S(82); }
float row_gap() { return S(12); }
float content_end() { return tabs::content_bottom() - S(68); }
float list_h() { return std::max(row_h(), content_end() - list_y()); }

bool declares_resource(const Addon &addon, const std::string &name) {
        if (name == "catalog" && !addon.catalogs.empty()) return true;
        for (const Json &r : addon.resources.elements()) {
                if (r.is_string() && r.as_string() == name) return true;
                if (r.is_object() && r["name"].as_string("") == name) return true;
        }
        return false;
}

std::string functions_label(const Addon &addon) {
        const struct {
                const char *resource;
                i18n::Text key;
        } entries[] = {
                { "catalog", i18n::Text::ADDONS_RESOURCE_CATALOG },
                { "meta", i18n::Text::ADDONS_RESOURCE_META },
                { "stream", i18n::Text::ADDONS_RESOURCE_STREAM },
                { "subtitles", i18n::Text::ADDONS_RESOURCE_SUBTITLES },
        };
        std::string result;
        for (const auto &entry : entries) {
                if (!declares_resource(addon, entry.resource)) continue;
                if (!result.empty()) result += " \xC2\xB7 ";
                result += i18n::tr(entry.key);
        }
        return result.empty() ? i18n::tr(i18n::Text::ADDONS_NO_FUNCTIONS) : result;
}

std::string display_name(const Addon &addon) {
        return addon.name.empty() ? addon.id : addon.name;
}
} // namespace

AddonsScreen::AddonsScreen(Fonts &fonts, Navigator &nav)
        : fonts_(fonts), navigator_(nav) {}

void AddonsScreen::update(double dt, uint32_t pressed) {
        if (bar_.handle_shoulders(pressed, fonts_, navigator_)) return;
        const int count = (int)stremio().addons().size();
        selected_ = std::clamp(selected_, 0, std::max(0, count - 1));

        if (on_bar_) {
                if (bar_.handle_focused(pressed, fonts_, navigator_)) return;
                if ((pressed & platform::BUTTON_UP) && count > 0) on_bar_ = false;
        } else {
                if (pressed & platform::BUTTON_UP) selected_ = std::max(0, selected_ - 1);
                if (pressed & platform::BUTTON_DOWN) {
                        if (selected_ + 1 < count) ++selected_;
                        else on_bar_ = true;
                }
                if (pressed & platform::BUTTON_BACK) on_bar_ = true;
        }

        focus_.resize(count, 0.0f);
        for (int i = 0; i < count; ++i) {
                focus_[i] = ui::approach(
                        focus_[i], !on_bar_ && i == selected_ ? 1.0f : 0.0f, dt);
        }
        if (count > 0) {
                scroll_ = ui::follow(
                        scroll_, selected_ * (row_h() + row_gap()),
                        row_h(), list_h(), dt);
        } else {
                scroll_ = 0;
        }
        bar_.update(dt, on_bar_);
}

void AddonsScreen::draw(Renderer &r) {
        const Stremio &s = stremio();
        const auto &addons = s.addons();
        r.text(fonts_.page_header, i18n::tr(i18n::Text::ADDONS),
                MARGIN, S(96), theme::BASE_CONTENT);

        std::string status;
        if (!s.addons_error().empty()) {
                status = std::string(i18n::tr(i18n::Text::ADDONS_LOAD_ERROR))
                        + s.addons_error();
        } else if (!s.addons_loaded()) {
                status = i18n::tr(i18n::Text::LOADING_ADDONS);
        } else {
                status = std::string(i18n::tr(i18n::Text::ADDONS_INSTALLED_PREFIX))
                        + std::to_string(addons.size());
        }
        r.text(fonts_.body, fonts_.body.ellipsize(status, W - 2 * MARGIN),
                MARGIN, S(135),
                s.addons_error().empty() ? theme::BASE_CONTENT_MUTED : theme::ERROR);

        for (size_t i = 0; i < addons.size(); ++i) {
                const float y = list_y() + i * (row_h() + row_gap()) - scroll_;
                if (y < list_y() - 1 || y + row_h() > list_y() + list_h() + 1) continue;
                const float f = i < focus_.size() ? focus_[i] : 0.0f;
                const bool selected = (int)i == selected_;
                const float radius = theme::radius_field();
                ui::begin_focus(r, list_x(), y, list_w(), row_h(), radius, f, 1.02f);
                r.rounded_rect(list_x(), y, list_w(), row_h(), radius,
                        selected ? mix(theme::BASE_200, theme::PRIMARY, 0.30f) : theme::BASE_200);
                r.text(fonts_.card_title,
                        fonts_.card_title.ellipsize(display_name(addons[i]), list_w() - S(48)),
                        list_x() + S(24), y + row_h() / 2 + S(9), theme::BASE_CONTENT);
                ui::end_focus(r, list_x(), y, list_w(), row_h(), radius, f);
        }

        if (addons.empty()) {
                if (s.addons_loaded() && s.addons_error().empty()) {
                        r.text(fonts_.body, i18n::tr(i18n::Text::ADDONS_EMPTY),
                                MARGIN, list_y() + S(40), theme::BASE_CONTENT_MUTED);
                }
        } else {
                const int index = std::clamp(selected_, 0, (int)addons.size() - 1);
                const Addon &addon = addons[index];
                const float x = list_x() + list_w() + S(32);
                const float w = W - MARGIN - x;
                const float pad = S(24);
                const float text_w = w - 2 * pad;
                r.rounded_rect(x, list_y(), w, list_h(),
                        theme::radius_box(), theme::BASE_200);

                float y = list_y() + S(42);
                r.text(fonts_.section, fonts_.section.ellipsize(display_name(addon), text_w),
                        x + pad, y, theme::BASE_CONTENT);
                y += S(36);
                if (!addon.version.empty()) {
                        const std::string version =
                                std::string(i18n::tr(i18n::Text::ADDONS_VERSION_PREFIX))
                                + addon.version;
                        r.text(fonts_.body, fonts_.body.ellipsize(version, text_w),
                                x + pad, y, theme::BASE_CONTENT_MUTED);
                        y += S(36);
                }
                r.text(fonts_.card_title, i18n::tr(i18n::Text::ADDONS_FUNCTIONS),
                        x + pad, y, theme::BASE_CONTENT);
                y += S(30);
                for (const auto &line : fonts_.body.wrap(functions_label(addon), text_w, 3)) {
                        r.text(fonts_.body, line, x + pad, y, theme::BASE_CONTENT_MUTED);
                        y += S(28);
                }
                y += S(20);
                const std::string description = addon.description.empty()
                        ? i18n::tr(i18n::Text::ADDONS_NO_DESCRIPTION) : addon.description;
                const float bottom = list_y() + list_h() - pad;
                const int max_lines = std::max(0, (int)((bottom - y) / S(28)));
                if (max_lines > 0) {
                        for (const auto &line : fonts_.body.wrap(description, text_w, max_lines)) {
                                r.text(fonts_.body, line, x + pad, y, theme::BASE_CONTENT);
                                y += S(28);
                        }
                }
        }

        r.text(fonts_.small,
                fonts_.small.ellipsize(i18n::tr(i18n::Text::ADDONS_READ_ONLY), W - 2 * MARGIN),
                MARGIN, tabs::content_bottom() - S(24), theme::BASE_CONTENT_MUTED);
        bar_.draw(r, fonts_);
}
