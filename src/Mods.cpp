#include <spdlog/spdlog.h>

#include "IntegrityCheckBypass.hpp"
#include "PositionHooks.hpp"
#include "DeveloperTools.hpp"
#include "Speedrun.h"

#include "Mods.hpp"
#include "ObjectExplorer.hpp"

Mods::Mods()
{
#ifdef RE3
    m_mods.emplace_back(std::make_unique<IntegrityCheckBypass>());
#endif

    m_mods.emplace_back(std::make_unique<PositionHooks>());
    m_mods.emplace_back(std::make_unique<Speedrun>());

#ifdef DEVELOPER
    m_mods.emplace_back(std::make_unique<DeveloperTools>());
#endif
}

std::optional<std::string> Mods::on_initialize() const {
    for (auto &mod : m_mods) {

        if (auto e = mod->on_initialize(); e != std::nullopt) {
            return e;
        }
    }
    utility::Config cfg{"re2_fw_config.txt"};

    for (auto &mod : m_mods) {
        mod->on_config_load(cfg);
    }

    return std::nullopt;
}

void Mods::on_frame() const {
    for (auto &mod : m_mods) {
        mod->on_frame();
    }
}

void Mods::on_draw_ui() const {
    for (auto &mod : m_mods) {
        mod->on_draw_ui();
    }
}

