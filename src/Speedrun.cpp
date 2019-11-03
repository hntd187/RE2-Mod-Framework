#include "REFramework.hpp"
#include "Speedrun.h"
#include <iomanip>
#include <sstream>
#include <chrono>
#include <spdlog/fmt/fmt.h>

const static std::stringstream &display(std::stringstream &os, std::chrono::nanoseconds ns) {
    char fill = os.fill();
    os.fill('0');
    auto h = std::chrono::duration_cast<std::chrono::hours>(ns);
    ns -= h;
    auto m = std::chrono::duration_cast<std::chrono::minutes>(ns);
    ns -= m;
    auto s = std::chrono::duration_cast<std::chrono::seconds>(ns);
    os << setw(2) << h.count() << "h:"
       << setw(2) << m.count() << "m:"
       << setw(2) << s.count() << 's';
    os.fill(fill);
    return os;
}

const static chrono::nanoseconds get_nanos(REManagedObject *bh, const string &key) {
    const auto t = utility::REManagedObject::getField<uint64_t>(bh, key);
    const auto m = chrono::duration(chrono::microseconds(t));
    return chrono::duration_cast<chrono::nanoseconds>(m);
}

const static ImGradient createColors() {
    auto progressColors = ImGradient();
    progressColors.addMark(0.0f, ImColor(200, 0, 0));
    progressColors.addMark(1.0f, ImColor(0, 200, 0));

    const auto base_level = 200;
    for (auto i = 0.01f; i < 0.99f; i += 0.01f) {
        const float red = base_level - (base_level * i);
        const float green = base_level - red;
        spdlog::info("i: {}, Red: {}, Green: {}", i, red, green);
        progressColors.addMark(i, ImColor(red, green, 0.0f));
    }
    progressColors.refreshCache();
    return progressColors;
}

std::optional<std::string> Speedrun::onInitialize() {
    progressColors = createColors();
    return Mod::onInitialize();
}

void Speedrun::onFrame() {
    if (enabled->value()) {
        drawStats();
    }
}

void Speedrun::onDrawUI() {
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_FirstUseEver);
    if (!ImGui::CollapsingHeader(getName().data())) {
        return;
    }
    enabled->draw("Enabled");
    ingame->draw("In Game Time");
}

void Speedrun::onConfigLoad(const utility::Config &cfg) {
    enabled->configLoad(cfg);
}

void Speedrun::onConfigSave(utility::Config &cfg) {
    enabled->configLoad(cfg);
}

void Speedrun::drawStats() {
    auto &globals = *g_framework->getGlobals();

    auto clock = globals.get<REBehavior>("app.ropeway.GameClock");
    auto rank = globals.get<REBehavior>("app.ropeway.GameRankSystem");
    auto player = globals.get<REBehavior>("app.ropeway.PlayerManager");

    auto player_condition = utility::REManagedObject::getField<REBehavior *>(player, "CurrentPlayerCondition");
    auto current_health = utility::REManagedObject::getField<signed int>(player_condition, "CurrentHitPoint");
    auto current_pct = utility::REManagedObject::getField<float>(player_condition, "HitPointPercentage");

    auto actual_time_nanos = get_nanos(clock, "ActualRecordTime");
    auto inv_time_nanos = get_nanos(clock, "InventorySpendingTime");
    auto system_time_nanos = get_nanos(clock, "SystemElapsedTime");

    auto rank_points = utility::REManagedObject::getField<float>(rank, "RankPoint");
    auto current_rank = utility::REManagedObject::getField<signed int>(rank, "GameRank");
    float color[4];
    progressColors.getColorAt(current_pct / 100.0f, color);

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("", &enabled->value(), windowFlags);
    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = ImColor(color[0], color[1], color[2]);
    ImGui::Separator();
    ImGui::LabelText("System Time", "%llu", system_time_nanos.count());
    std::stringstream os;
    auto o = display(os, actual_time_nanos).str();
    ImGui::LabelText("Game Time", "%s", o.data());
    os = std::stringstream();
    o = display(os, inv_time_nanos).str();
    ImGui::LabelText("Inventory Time", "%s", o.data());
    ImGui::Separator();

    ImGui::LabelText("Current Health", "%i", current_health);
    ImGui::LabelText("Current Pct", "%0.f%%", current_pct);
    ImGui::ProgressBar(current_pct / 100.0f);
    ImGui::Separator();

    ImGui::LabelText("Current Rank", "%i", current_rank);
    ImGui::LabelText("Rank Points", "%1.f", rank_points);

    auto enemy_manager = globals.get<RopewayEnemyManager>("app.ropeway.EnemyManager");
    if (enemy_manager == nullptr) {
        return;
    }

    auto enemy_controllers = enemy_manager->enemyControllers;
    if (enemy_controllers == nullptr || enemy_controllers->data == nullptr) {
        return;
    }
    int COLUMNS = 5;
    ImGui::Columns(COLUMNS, "Health", false);
    ImGui::Separator();
    for (auto i = 0; i < enemy_controllers->data->numElements; ++i) {
        auto enemy_controller = utility::REArray::getElement<RopewayEnemyController>(enemy_controllers->data, i);
        REBehavior *hitpoint_controller = nullptr;
        if (enemy_controller == nullptr) {
            break;
        }
        if (!utility::REManagedObject::isManagedObject(enemy_controller)) {
            continue;
        }
        for (auto component = enemy_controller->childComponent;
             component != nullptr && component != enemy_controller; component = component->childComponent) {
            if (utility::REManagedObject::isA(component, "app.ropeway.HitPointController")) {
                hitpoint_controller = (REBehavior *) component;
                break;
            }
        }
        if (hitpoint_controller == nullptr) {
            continue;
        }

        auto region = ImGui::GetContentRegionAvail();
        auto box_demn = ImVec2(region.x, 20.0f);
        auto ratio = utility::REManagedObject::getField<float>(hitpoint_controller, "HitPointRatio");
        float c[4];
        progressColors.getColorAt(ratio, c);
        spdlog::info("Alpha is: {}", c[3]);
        ImGui::ColorButton(fmt::format("{}%", ratio).data(), ImVec4(c[0], c[1], c[2], 1.0f),
                           ImGuiColorEditFlags_NoTooltip, box_demn);
        if ((i % COLUMNS) != 0) {
            ImGui::SameLine((i * region.x) / COLUMNS);
        }
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::End();
}
