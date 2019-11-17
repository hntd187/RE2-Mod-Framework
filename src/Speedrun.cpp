#include "REFramework.hpp"
#include "Speedrun.h"
#include <iomanip>
#include <sstream>
#include <chrono>
#include <utility>

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

const static ImColor createColor(const float i) {
    if (i == 1.0f) {
        return IM_COL32(37, 97, 68, 255);
    } else if (i < 1.0f && i >= 0.66f) {
        return IM_COL32(51, 72, 24, 255);
    } else if (i < 0.66f && i >= 0.30f) {
        return IM_COL32(94, 72, 27, 255);
    } else if (i < 0.30f) {
        return IM_COL32(136, 1, 27, 255);
    }
}

std::optional<std::string> Speedrun::onInitialize() {
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
    locked->draw("Lock Window");
    ingame->draw("In Game Time");
    health->draw("Health");
    game_rank->draw("Game Rank");
    local_enemies->draw("Nearby Enemies");

    ImGui::BulletText("Drag and drop to re-order");
    int move_from = -1, move_to = -1;
    for (int n = 0; n < 4; n++) {
        ImGui::Selectable(info_labels[info_order[n] - 1], info_order[n]);

        ImGuiDragDropFlags src_flags =
                ImGuiDragDropFlags_SourceNoDisableHover | ImGuiDragDropFlags_SourceNoHoldToOpenOthers |
                ImGuiDragDropFlags_SourceNoPreviewTooltip;
        if (ImGui::BeginDragDropSource(src_flags)) {
            ImGui::SetDragDropPayload("DND_DEMO_NAME", &n, sizeof(int));
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            ImGuiDragDropFlags target_flags =
                    ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DND_DEMO_NAME", target_flags)) {
                move_from = *(const int *) payload->Data;
                move_to = n;
            }
            ImGui::EndDragDropTarget();
        }
    }

    if (move_from != -1 && move_to != -1) {
        std::swap(info_order[move_from], info_order[move_to]);
        ImGui::SetDragDropPayload("DND_DEMO_NAME", &move_to, sizeof(int));
    }
}

void Speedrun::onConfigLoad(const utility::Config &cfg) {
    enabled->configLoad(cfg);
    ingame->configLoad(cfg);
    health->configLoad(cfg);
    game_rank->configLoad(cfg);
    local_enemies->configLoad(cfg);
}

void Speedrun::onConfigSave(utility::Config &cfg) {
    enabled->configSave(cfg);
    ingame->configSave(cfg);
    health->configSave(cfg);
    game_rank->configSave(cfg);
    local_enemies->configSave(cfg);
}

void Speedrun::drawStats() {

    auto &globals = *g_framework->getGlobals();
    auto clock = globals.get<REBehavior>("app.ropeway.GameClock");
    auto system_time_nanos = get_nanos(clock, "SystemElapsedTime");
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("", &enabled->value(), windowFlags(locked->value()));
    ImGui::LabelText("System Time", "%lld", system_time_nanos.count());
    ImGui::Separator();
    for (auto i = 0; i < 4; i++) {
        switch (info_order[i]) {
            case 1:
                if (ingame->value()) drawIngameTime(clock);
                continue;
            case 2:
                if (health->value()) {
                    auto player = globals.get<REBehavior>("app.ropeway.PlayerManager");
                    drawHealth(player);
                }
                continue;
            case 3:
                if (game_rank->value()) {
                    auto rank = globals.get<REBehavior>("app.ropeway.GameRankSystem");
                    drawGameRank(rank);
                }
                continue;
            case 4:
                if (local_enemies->value()) {
                    auto enemy_manager = globals.get<RopewayEnemyManager>("app.ropeway.EnemyManager");
                    drawEnemies(enemy_manager);
                }
                continue;
        }
        if (enabled->value() && !ingame->value() && !health->value() && !game_rank->value() &&
            !local_enemies->value()) {
            ImGui::Text("You disabled everything, but left the overlay enabled.");
        }

    }
    ImGui::End();
}

void Speedrun::drawIngameTime(REBehavior *clock) {
    auto actual_time_nanos = get_nanos(clock, "ActualRecordTime");
    auto inv_time_nanos = get_nanos(clock, "InventorySpendingTime");

    std::stringstream os;
    auto o = display(os, actual_time_nanos).str();
    ImGui::LabelText("Game Time", "%s", o.data());
    os = std::stringstream();
    o = display(os, inv_time_nanos).str();
    ImGui::LabelText("Inventory Time", "%s", o.data());
}

void Speedrun::drawHealth(REBehavior *player) {
    auto player_condition = utility::REManagedObject::getField<REBehavior *>(player, "CurrentPlayerCondition");
    auto current_health = utility::REManagedObject::getField<signed int>(player_condition, "CurrentHitPoint");
    auto current_pct = utility::REManagedObject::getField<float>(player_condition, "HitPointPercentage");

    ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = createColor(current_pct / 100.0f);
    ImGui::LabelText("Current Health", "%i", current_health);
    ImGui::LabelText("Current Pct", "%0.f%%", current_pct);
    ImGui::ProgressBar(current_pct / 100.0f);
}

void Speedrun::drawGameRank(REBehavior *rank) {
    auto rank_points = utility::REManagedObject::getField<float>(rank, "RankPoint");
    auto current_rank = utility::REManagedObject::getField<signed int>(rank, "GameRank");

    ImGui::LabelText("Current Rank", "%i", current_rank);
    ImGui::LabelText("Rank Points", "%1.f", rank_points);
}

void Speedrun::drawEnemies(RopewayEnemyManager *enemy_manager) {
    if (enemy_manager != nullptr) {
        auto enemy_controllers = enemy_manager->enemyControllers;
        if (enemy_controllers != nullptr && enemy_controllers->data != nullptr) {
            ImGui::Columns(COLUMNS, "Health", false);

            for (auto i = 0; i < enemy_controllers->data->numElements; ++i) {
                auto ec = utility::REArray::getElement<RopewayEnemyController>(enemy_controllers->data, i);
                REBehavior *hitpoint_controller = nullptr;
                if (ec == nullptr) break;
                if (!utility::REManagedObject::isManagedObject(ec)) continue;
                for (auto c = ec->childComponent; c != nullptr && c != ec; c = c->childComponent) {
                    if (utility::REManagedObject::isA(c, "app.ropeway.HitPointController")) {
                        hitpoint_controller = (REBehavior *) c;
                        break;
                    }
                }
                if (hitpoint_controller == nullptr) continue;
                auto region = ImGui::GetContentRegionAvail();
                auto ratio = utility::REManagedObject::getField<float>(hitpoint_controller, "HitPointRatio");
                ImGui::TextColored(createColor(ratio), "%.0f%%", ratio * 100.0);
                if ((i % COLUMNS) != 0) ImGui::SameLine((i * region.x) / COLUMNS);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
            ImGui::NewLine();
            ImGui::NewLine();
        }
    }
}


