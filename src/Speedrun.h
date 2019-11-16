#pragma once

#include <chrono>
#include "Mod.hpp"

using namespace std;

class Speedrun : public Mod {
public:
    [[nodiscard]] std::string_view getName() const override { return "Speedrun"; }

    std::optional<std::string> onInitialize() override;

    void onFrame() override;

    void onDrawUI() override;

    void onConfigLoad(const utility::Config &cfg) override;

    void onConfigSave(utility::Config &cfg) override;

    void drawStats();

    static void drawIngameTime(REBehavior *clock);

    static void drawHealth(REBehavior *health);

    static void drawGameRank(REBehavior *rank);

    static void drawEnemies(RopewayEnemyManager *enemies);

private:
    constexpr static const int COLUMNS = 5;
    constexpr static const char *info_labels[4] = {"IGT", "Health", "Rank", "Enemies"};

    int info_order[4] = {1, 2, 3, 4};

    static const int windowFlags(const bool locked) {
        auto base_flags = ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_NoDecoration |
                          ImGuiWindowFlags_NoBackground |
                          ImGuiWindowFlags_NoBringToFrontOnFocus |
                          ImGuiWindowFlags_NoFocusOnAppearing |
                          ImGuiWindowFlags_NoNav;
        return (locked) ? base_flags | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs : base_flags;
    }

    ModToggle::Ptr locked{ModToggle::create(generateName("Lock Window"), true)};
    ModToggle::Ptr enabled{ModToggle::create(generateName("Enabled"), false)};
    ModToggle::Ptr ingame{ModToggle::create(generateName("In Game Time"), true)};
    ModToggle::Ptr health{ModToggle::create(generateName("Health"), true)};
    ModToggle::Ptr game_rank{ModToggle::create(generateName("Rank"), true)};
    ModToggle::Ptr local_enemies{ModToggle::create(generateName("Local Enemies"), true)};

};