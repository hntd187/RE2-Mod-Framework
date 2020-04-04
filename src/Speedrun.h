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

    static void reset();

    void draw_stats();

    static void draw_ingame_time(REBehavior *clock);

    static void draw_health(REBehavior *player, bool draw_health = true);

    static void draw_game_rank(REBehavior *rank);

    static void draw_enemies(RopewayEnemyManager *enemy_manager, bool draw_bg = true);

private:
    constexpr static const int COLUMNS = 5;
    constexpr static const char *info_labels[4] = {"IGT", "Health", "Rank", "Enemies"};

    int info_order[4] = {1, 2, 3, 4};

#ifdef RE3
    constexpr static const auto PREFIX = "offline.";
#else
    constexpr static const auto PREFIX = "app.ropeway.";
#endif

    static auto make_name(const std::string& key) {
        return fmt::format("{}{}", PREFIX, key);
    }

    static int windowFlags(const bool locked) {
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
    const ModKey::Ptr reset_btn{ModKey::create(generate_name("reset"), DIKEYBOARD_F9)};

};