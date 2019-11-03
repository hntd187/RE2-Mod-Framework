#pragma once

#include <chrono>
#include "re2-imgui/imgui_color_gradient.h"
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

private:

    ImGradient progressColors;
    const int windowFlags = ImGuiWindowFlags_AlwaysAutoResize |
                            ImGuiWindowFlags_NoBackground |
                            ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoMove;

    ModToggle::Ptr enabled{ModToggle::create(generateName("Enabled"), false)};
    ModToggle::Ptr ingame{ModToggle::create(generateName("In Game Time"), true)};
};