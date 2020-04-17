#pragma once

#include <imgui.h>

#include <chrono>
#include "Mod.hpp"

class Speedrun : public Mod {
public:

    [[nodiscard]] std::string_view get_name() const override { return "Speedrun"; }

    std::optional<std::string> on_initialize() override;

    void on_frame() override;

    void on_draw_ui() override;

    void on_config_load(const utility::Config &cfg) override;

    void on_config_save(utility::Config &cfg) override;

    void draw_stats();

    static void reset();

    static void draw_ingame_time(REBehavior *clock);

    void draw_health(REBehavior *player, bool draw_health = true);

    static void draw_game_rank(REBehavior *rank);

    void draw_enemies(RopewayEnemyManager *enemy_manager, bool draw_bg = true);

    float sc = 1.0f / 255.0f;
    float fine[3] = {37 * sc, 97 * sc, 68 * sc};
    float kinda_fine[3] = {51 * sc, 72 * sc, 24 * sc};
    float caution[3] = {94 * sc, 72 * sc, 27 * sc};
    float extra_caution[3] = {136 * sc, 1 * sc, 27 * sc};
    float danger[3] = {136 * sc, 1 * sc, 27 * sc};

    ImColor create_color(const float i) {
        if (i >= 1.0f) return ImColor(fine[0], fine[1], fine[2], 1.0f);                                // Fine
        if (i < 1.0f && i >= 0.66f) return ImColor(kinda_fine[0], kinda_fine[1], kinda_fine[2], 1.0f); // That kinda fine, but not color
        if (i < 0.66f && i >= 0.30f) return ImColor(caution[0], caution[1], caution[2], 1.0f);         // Caution
        if (i < 0.30f) return ImColor(extra_caution[0], extra_caution[1], extra_caution[2], 1.0f);     // Extra Caution
        return ImColor(danger[0], danger[1], danger[2], 1.0f);                                         // Danger
    }

    void make_button(const float ratio, const bool draw_bg) {
        const ImColor color = create_color(ratio);
        if (draw_bg) {
            ImGui::PushStyleColor(ImGuiCol_Button, (ImU32) color);
            ImGui::Button(fmt::format("{:.0%}", ratio).data(), ImVec2(70, 30));
            ImGui::PopStyleColor(1);
        } else {
            ImGui::TextColored(color, "%.0f%%", ratio * 100.0);
        }
    }

private:
    constexpr static const int COLUMNS = 5;
    constexpr static const char *INFO_LABELS[4] = {"IGT", "Health", "Rank", "Enemies"};

    int info_order[4] = {1, 2, 3, 4};

    static int window_flags(const bool locked) {
        auto base_flags = ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_NoDecoration |
                          ImGuiWindowFlags_NoBackground |
                          ImGuiWindowFlags_NoBringToFrontOnFocus |
                          ImGuiWindowFlags_NoFocusOnAppearing |
                          ImGuiWindowFlags_NoNav;
        return (locked) ? base_flags | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs : base_flags;
    }

    const ModToggle::Ptr locked{ModToggle::create(generate_name("Lock Window"), true)};
    const ModToggle::Ptr enabled{ModToggle::create(generate_name("Enabled"), false)};
    const ModToggle::Ptr ingame{ModToggle::create(generate_name("In Game Time"), true)};
    const ModToggle::Ptr health{ModToggle::create(generate_name("Health"), true)};
    const ModToggle::Ptr game_rank{ModToggle::create(generate_name("Rank"), true)};
    const ModToggle::Ptr local_enemies{ModToggle::create(generate_name("Local Enemies"), true)};
    const ModToggle::Ptr health_bar{ModToggle::create(generate_name("Health Bar"), true)};
    const ModToggle::Ptr colored_buttons{ModToggle::create(generate_name("Colored Backgrounds for Enemy Health"), true)};
    const ModKey::Ptr reset_btn{ModKey::create(generate_name("Reset"), 67)};
    const ModSlider::Ptr lod_params{ModSlider::create(generate_name("LOD Distance"), 0, 100)};

};