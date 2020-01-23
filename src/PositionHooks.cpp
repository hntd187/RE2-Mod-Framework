#include "Mods.hpp"
#include "REFramework.hpp"
#include "utility/Scan.hpp"

#include "PositionHooks.hpp"

PositionHooks* g_hook = nullptr;

PositionHooks::PositionHooks() {
    g_hook = this;
}

std::optional<std::string> PositionHooks::onInitialize() {
    auto game = g_framework->getModule().as<HMODULE>();

    // The 48 8B 4D 40 bit might change.
    // Version 1.0 jmp stub: game+0x1dc7de0
    auto updateTransformCall = utility::scan(game, "E8 ? ? ? ? 48 8B 5B ? 48 85 DB 75 ? 48 8B 4D 40 48 ? ?");

    if (!updateTransformCall) {
        return "Unable to find UpdateTransform pattern.";
    }

    auto updateTransform = utility::calculateAbsolute(*updateTransformCall + 1);
    spdlog::info("UpdateTransform: {:x}", updateTransform);

    // Can be found by breakpointing RETransform's worldTransform
    m_updateTransformHook = std::make_unique<FunctionHook>(updateTransform, &updateTransformHook);

    if (!m_updateTransformHook->create()) {
        return "Failed to hook UpdateTransform";
    }

    // Version 1.0 jmp stub: game+0xB4685A0
    auto updateCameraController = utility::scan(game, "40 55 56 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? 00 00 48 8B 41 50");

    // Can be found by breakpointing camera controller's worldPosition
    m_updateCameraControllerHook = std::make_unique<FunctionHook>(*updateCameraController, &updateCameraControllerHook);

    if (!m_updateCameraControllerHook->create()) {
        return "Failed to hook UpdateCameraController";
    }

    // Version 1.0 jmp stub: game+0xCF2510
    // Version 1.0 function: game+0xB436230
    auto updateCameraController2 = utility::scan(game, "40 53 57 48 81 EC ? ? ? ? 48 ? ? ? 48 ? ? 48 ? ? ? ? 00 00");

    if (!updateCameraController2) {
        return "Unable to find UpdateCameraController2 pattern.";
    }

    spdlog::info("UpdateCameraController2: {:x}", *updateCameraController2);

    // Can be found by breakpointing camera controller's worldRotation
    m_updateCameraController2Hook = std::make_unique<FunctionHook>(*updateCameraController2, &updateCameraController2Hook);

    if (!m_updateCameraController2Hook->create()) {
        return "Failed to hook UpdateCameraController2";
    }

    return Mod::onInitialize();
}

void* PositionHooks::updateTransformHook_Internal(RETransform* t, uint8_t a2, uint32_t a3) {
    if (!g_framework->isReady()) {
        return m_updateTransformHook->getOriginal<decltype(updateTransformHook)>()(t, a2, a3);
    }

    auto& mods = g_framework->getMods()->getMods();

    for (auto& mod : mods) {
        mod->onPreUpdateTransform(t);
    }

    auto ret = m_updateTransformHook->getOriginal<decltype(updateTransformHook)>()(t, a2, a3);

    for (auto& mod : mods) {
        mod->onUpdateTransform(t);
    }

    return ret;
}

void* PositionHooks::updateTransformHook(RETransform* t, uint8_t a2, uint32_t a3) {
    return g_hook->updateTransformHook_Internal(t, a2, a3);
}

void* PositionHooks::updateCameraControllerHook_Internal(void* a1, RopewayPlayerCameraController* cameraController) {
    if (!g_framework->isReady()) {
        return m_updateCameraControllerHook->getOriginal<decltype(updateCameraControllerHook)>()(a1, cameraController);
    }

    auto& mods = g_framework->getMods()->getMods();

    for (auto& mod : mods) {
        mod->onPreUpdateCameraController(cameraController);
    }

    auto ret = m_updateCameraControllerHook->getOriginal<decltype(updateCameraControllerHook)>()(a1, cameraController);

    for (auto& mod : mods) {
        mod->onUpdateCameraController(cameraController);
    }

    return ret;
}

void* PositionHooks::updateCameraControllerHook(void* a1, RopewayPlayerCameraController* cameraController) {
    return g_hook->updateCameraControllerHook_Internal(a1, cameraController);
}

void* PositionHooks::updateCameraController2Hook_Internal(void* a1, RopewayPlayerCameraController* cameraController) {
    if (!g_framework->isReady()) {
        return m_updateCameraController2Hook->getOriginal<decltype(updateCameraController2Hook)>()(a1, cameraController);
    }

    auto& mods = g_framework->getMods()->getMods();

    for (auto& mod : mods) {
        mod->onPreUpdateCameraController2(cameraController);
    }

    auto ret = m_updateCameraController2Hook->getOriginal<decltype(updateCameraController2Hook)>()(a1, cameraController);

    for (auto& mod : mods) {
        mod->onUpdateCameraController2(cameraController);
    }

    return ret;
}

void* PositionHooks::updateCameraController2Hook(void* a1, RopewayPlayerCameraController* cameraController) {
    return g_hook->updateCameraController2Hook_Internal(a1, cameraController);
}

