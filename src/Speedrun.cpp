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
    os << std::setw(2) << h.count() << "h:"
       << std::setw(2) << m.count() << "m:"
       << std::setw(2) << s.count() << 's';
    os.fill(fill);
    return os;
}

static std::chrono::nanoseconds get_nanos(REManagedObject *bh, const std::string &key) {
    const auto t = utility::re_managed_object::get_field<uint64_t>(bh, key);
    const auto m = std::chrono::duration(std::chrono::microseconds(t));
    return std::chrono::duration_cast<std::chrono::nanoseconds>(m);
}

static ImColor createColor(const float i) {
    if (i >= 1.0f) {
        return IM_COL32(37, 97, 68, 255);
    } else if (i < 1.0f && i >= 0.66f) {
        return IM_COL32(51, 72, 24, 255);
    } else if (i < 0.66f && i >= 0.30f) {
        return IM_COL32(94, 72, 27, 255);
    } else if (i < 0.30f) {
        return IM_COL32(136, 1, 27, 255);
    }
    return IM_COL32(136, 1, 27, 255);
}

static void makeButton(const float ratio, const bool draw_bg) {
    const ImColor color = createColor(ratio);
    if (draw_bg) {
        ImGui::PushStyleColor(ImGuiCol_Button, (ImU32) color);
        ImGui::Button(fmt::format("{:.0%}", ratio).data(), ImVec2(70, 30));
        ImGui::PopStyleColor(1);
    } else {
        ImGui::TextColored(color, "%.0f%%", ratio * 100.0);
    }
}

std::optional<std::string> Speedrun::on_initialize() {
    return Mod::on_initialize();
}

void Speedrun::on_frame() {
    if (reset_btn->is_key_down_once()) {
        return reset();
    }
    if (enabled->value()) {
        draw_stats();
    }
}

void Speedrun::on_draw_ui() {
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_FirstUseEver);
    if (!ImGui::CollapsingHeader(get_name().data())) {
        return;
    }
    enabled->draw("Enabled");
    locked->draw("Lock Window");
    ingame->draw("In Game Time");
    health->draw("Health");
    game_rank->draw("Game Rank");
    local_enemies->draw("Nearby Enemies");
    health_bar->draw("Health Bar");
    colored_buttons->draw("Colored Backgrounds for Enemies");
    reset_btn->draw("Reset Bind");

    ImGui::NewLine();
    ImGui::NewLine();
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

void Speedrun::on_config_load(const utility::Config &cfg) {
    enabled->config_load(cfg);
    ingame->config_load(cfg);
    health->config_load(cfg);
    game_rank->config_load(cfg);
    local_enemies->config_load(cfg);
    reset_btn->config_load(cfg);
}

void Speedrun::on_config_save(utility::Config &cfg) {
    enabled->config_save(cfg);
    ingame->config_save(cfg);
    health->config_save(cfg);
    game_rank->config_save(cfg);
    local_enemies->config_save(cfg);
    reset_btn->config_save(cfg);
}

void Speedrun::draw_stats() {
    auto &globals = *g_framework->get_globals();
    auto clock = globals.get<REBehavior>(make_name("GameClock"));
    auto system_time_nanos = get_nanos(clock, "SystemElapsedTime");
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("", &enabled->value(), windowFlags(locked->value()));
    ImGui::LabelText("System Time", "%lld", system_time_nanos.count());
    ImGui::Separator();
    for (int i : info_order) {
        switch (i) {
            case 1:
                if (ingame->value()) draw_ingame_time(clock);
                continue;
            case 2:
                if (health->value()) {
                    auto player = globals.get<REBehavior>(make_name("PlayerManager"));
                    draw_health(player, health_bar->value());
                }
                continue;
            case 3:
                if (game_rank->value()) {
                    auto rank = globals.get<REBehavior>(make_name("GameRankSystem"));
                    draw_game_rank(rank);
                }
                continue;
            case 4:
                if (local_enemies->value()) {
                    auto enemy_manager = globals.get<RopewayEnemyManager>(make_name("EnemyManager"));
                    draw_enemies(enemy_manager, colored_buttons->value());
                }
                continue;
            default:
                if (enabled->value() && !ingame->value() && !health->value() && !game_rank->value() &&
                    !local_enemies->value()) {
                    ImGui::Text("You disabled everything, but left the overlay enabled.");
                }
                continue;
        }
    }
    ImGui::End();
}

void Speedrun::draw_ingame_time(REBehavior *clock) {
    auto actual_time_nanos = get_nanos(clock, "ActualRecordTime");
    auto inv_time_nanos = get_nanos(clock, "InventorySpendingTime");

    std::stringstream os;
    auto o = display(os, actual_time_nanos).str();
    ImGui::LabelText("Game Time", "%s", o.data());
    os = std::stringstream();
    o = display(os, inv_time_nanos).str();
    ImGui::LabelText("Inventory Time", "%s", o.data());
}

void Speedrun::draw_health(REBehavior *player, const bool draw_health) {
    auto player_condition = utility::re_managed_object::get_field<REBehavior *>(player, "CurrentPlayerCondition");
    auto current_health = utility::re_managed_object::get_field<signed int>(player_condition, "CurrentHitPoint");
    auto current_pct = utility::re_managed_object::get_field<float>(player_condition, "HitPointPercentage");

    ImGui::LabelText("Current Health", "%i", current_health);
    ImGui::LabelText("Current Pct", "%0.f%%", current_pct);
    if (draw_health) {
        ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram] = createColor(current_pct / 100.0f);
        ImGui::ProgressBar(current_pct / 100.0f);
    }
}

void Speedrun::draw_game_rank(REBehavior *rank) {
    auto rank_points = utility::re_managed_object::get_field<float>(rank, "RankPoint");
    auto current_rank = utility::re_managed_object::get_field<signed int>(rank, "GameRank");

    ImGui::LabelText("Current Rank", "%i", current_rank);
    ImGui::LabelText("Rank Points", "%1.f", rank_points);
}

void Speedrun::draw_enemies(RopewayEnemyManager *enemy_manager, const bool draw_bg) {
    if (enemy_manager != nullptr) {
        auto enemy_controllers = enemy_manager->enemyControllers;
        if (enemy_controllers != nullptr && enemy_controllers->data != nullptr) {
            ImGui::Columns(COLUMNS, "Health", false);

            for (auto i = 0; i < enemy_controllers->data->numElements; ++i) {
                auto ec = utility::re_array::get_element<RopewayEnemyController>(enemy_controllers->data, i);
                REBehavior *hitpoint_controller = nullptr;
                if (ec == nullptr) break;
                if (!utility::re_managed_object::is_managed_object(ec)) continue;
                for (auto c = ec->childComponent; c != nullptr && c != ec; c = c->childComponent) {
                    if (utility::re_managed_object::is_a(c, "HitPointController")) {
                        hitpoint_controller = (REBehavior *) c;
                        break;
                    }
                }
                if (hitpoint_controller == nullptr) continue;
                auto region = ImGui::GetContentRegionAvail();
                auto ratio = utility::re_managed_object::get_field<float>(hitpoint_controller, "HitPointRatio");
                makeButton(ratio, draw_bg);
                if ((i % COLUMNS) != 0) ImGui::SameLine((i * region.x) / COLUMNS);
                ImGui::NextColumn();
            }
            ImGui::Columns(1);
            ImGui::NewLine();
            ImGui::NewLine();
        }
    }
}

void Speedrun::reset() {
    auto &globals = *g_framework->get_globals();
    auto rank = globals.get<REBehavior>(make_name("gamemastering.MainFlowManager"));
    auto inGame = utility::re_managed_object::get_field<signed int>(rank, "IsInGame");
    auto gameOver = utility::re_managed_object::get_field<signed int>(rank, "IsInGameOver");
    auto resetTitle = utility::re_managed_object::get_field<signed int>(rank, "IsInResetTitle");
    auto wakeUp = utility::re_managed_object::get_field<signed int>(rank, "IsInWakeUp");

    if (inGame && !gameOver) {
        spdlog::info("inGame: {}, gameOver: {}, resetTitle: {}, wakeUp: {}", inGame, gameOver, resetTitle, wakeUp);
        utility::re_managed_object::get_field<REComponent *>(rank, "goResetGameToTitle");
    }
}


