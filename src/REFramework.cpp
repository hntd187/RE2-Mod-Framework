#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <imgui/imgui.h>

// ours with XInput removed
#include "re2-imgui/imgui_impl_win32.h"
#include "re2-imgui/imgui_impl_dx11.h"

#include "utility/Module.hpp"
#include "utility/DroidFont.cpp"

#include "sdk/REGlobals.hpp"
#include "Mods.hpp"

#include "LicenseStrings.hpp"
#include "REFramework.hpp"

std::unique_ptr<REFramework> g_framework{};

REFramework::REFramework()
        : m_logger{spdlog::basic_logger_mt("REFramework", "re2_framework_log.txt", true)},
          m_gameModule{GetModuleHandle(0)} {
    spdlog::set_default_logger(m_logger);
    spdlog::flush_on(spdlog::level::info);
    spdlog::info("REFramework entry");

#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    m_d3d11Hook = std::make_unique<D3D11Hook>();
    m_d3d11Hook->onPresent([this](D3D11Hook &hook) { onFrame(); });
    m_d3d11Hook->onResizeBuffers([this](D3D11Hook &hook) { onReset(); });

    if ((m_valid = m_d3d11Hook->hook())) {
        spdlog::info("Hooked D3D11");
    }
}

REFramework::~REFramework() = default;

void REFramework::onFrame() {
    spdlog::debug("OnFrame");

    if (!m_initialized) {
        if (!initialize()) {
            spdlog::error("Failed to initialize REFramework");
            return;
        }

        spdlog::info("REFramework initialized");
        m_initialized = true;
        return;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_error.empty() && m_gameDataInitialized) {
        m_mods->onFrame();
    }

    drawUI();

    ImGui::EndFrame();
    ImGui::Render();

    ID3D11DeviceContext *context = nullptr;
    m_d3d11Hook->getDevice()->GetImmediateContext(&context);

    context->OMSetRenderTargets(1, &m_mainRenderTargetView, NULL);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void REFramework::onReset() {
    spdlog::info("Reset!");

    // Crashes if we don't release it at this point.
    cleanupRenderTarget();
    m_initialized = false;
}

bool REFramework::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!m_initialized) {
        return true;
    }

    if (m_drawUI && ImGui_ImplWin32_WndProcHandler(wnd, message, wParam, lParam) != 0) {
        // If the user is interacting with the UI we block the message from going to the game.
        auto &io = ImGui::GetIO();

        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) {
            return false;
        }
    }

    return true;
}

// this is unfortunate.
void REFramework::onDirectInputKeys(const std::array<uint8_t, 256> &keys) {
    if (keys[m_menuKey] && m_lastKeys[m_menuKey] == 0) {
        std::lock_guard _{m_inputMutex};
        m_drawUI = !m_drawUI;

        // Save the config if we close the UI
        if (!m_drawUI && m_gameDataInitialized) {
            saveConfig();
        }
    }

    m_lastKeys = keys;
}

void REFramework::saveConfig() {
    spdlog::info("Saving config re2_fw_config.txt");

    utility::Config cfg{};

    for (auto &mod : m_mods->getMods()) {
        mod->onConfigSave(cfg);
    }

    if (!cfg.save("re2_fw_config.txt")) {
        spdlog::info("Failed to save config");
        return;
    }

    spdlog::info("Saved config");
}

void REFramework::drawUI() {
    std::lock_guard _{m_inputMutex};

    if (!m_drawUI) {
        m_dinputHook->acknowledgeInput();
        ImGui::GetIO().MouseDrawCursor = false;
        return;
    }

    auto &io = ImGui::GetIO();

    if (io.WantCaptureKeyboard) {
        m_dinputHook->ignoreInput();
    } else {
        m_dinputHook->acknowledgeInput();
    }

    ImGui::GetIO().MouseDrawCursor = true;

    ImGui::SetNextWindowPos(ImVec2(500, 50), ImGuiCond_::ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_::ImGuiCond_Once);

    ImGui::Begin("RE2 Speedrun Overlay", &m_drawUI);
    ImGui::Text("Menu Key: Insert");

    drawAbout();

    if (m_error.empty() && m_gameDataInitialized) {
        m_mods->onDrawUI();
    } else if (!m_gameDataInitialized) {
        ImGui::TextWrapped("REFramework is currently initializing...");
    } else if (!m_error.empty()) {
        ImGui::TextWrapped("REFramework error: %s", m_error.c_str());
    }

    ImGui::End();
}

void REFramework::drawAbout() {
    if (!ImGui::CollapsingHeader("About")) {
        return;
    }

    ImGui::TreePush("About");
    ImGui::Text("Author: hntd187");
    ImGui::Text("https://github.com/hntd187/RE2-Speedrun-Overlay");
    ImGui::NewLine();
    ImGui::Text("Original work on the RE2 Framework by: praydog");
    ImGui::Text("https://github.com/praydog/RE2-Mod-Framework");
    if (ImGui::CollapsingHeader("Licenses")) {
        ImGui::TreePush("Licenses");
        if (ImGui::CollapsingHeader("glm")) ImGui::TextWrapped(license::glm);
        if (ImGui::CollapsingHeader("imgui")) ImGui::TextWrapped(license::imgui);
        if (ImGui::CollapsingHeader("minhook")) ImGui::TextWrapped(license::minhook);
        if (ImGui::CollapsingHeader("spdlog")) ImGui::TextWrapped(license::spdlog);
        ImGui::TreePop();
    }
    ImGui::TreePop();
}

bool REFramework::initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("Attempting to initialize");

    auto device = m_d3d11Hook->getDevice();
    auto swapChain = m_d3d11Hook->getSwapChain();

    // Wait.
    if (device == nullptr || swapChain == nullptr) {
        spdlog::info("Device or SwapChain null. DirectX 12 may be in use. A crash may occur.");
        return false;
    }

    ID3D11DeviceContext *context = nullptr;
    device->GetImmediateContext(&context);

    DXGI_SWAP_CHAIN_DESC swapDesc{};
    swapChain->GetDesc(&swapDesc);

    m_wnd = swapDesc.OutputWindow;

    // Explicitly call destructor first
    m_windowsMessageHook.reset();
    m_windowsMessageHook = std::make_unique<WindowsMessageHook>(m_wnd);
    m_windowsMessageHook->onMessage = [this](auto wnd, auto msg, auto wParam, auto lParam) {
        return onMessage(wnd, msg, wParam, lParam);
    };

    // just do this instead of rehooking because there's no point.
    if (m_firstFrame) {
        m_dinputHook = std::make_unique<DInputHook>(m_wnd);
    } else {
        m_dinputHook->setWindow(m_wnd);
    }

    spdlog::info("Creating render target");

    createRenderTarget();

    spdlog::info("Window Handle: {0:x}", (uintptr_t) m_wnd);
    spdlog::info("Initializing ImGui");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    spdlog::info("Initializing ImGui Win32");
    if (!ImGui_ImplWin32_Init(m_wnd)) {
        spdlog::error("Failed to initialize ImGui.");
        return false;
    }

    spdlog::info("Initializing ImGui D3D11");

    if (!ImGui_ImplDX11_Init(device, context)) {
        spdlog::error("Failed to initialize ImGui.");
        return false;
    }

    ImGuiIO &io = ImGui::GetIO();
    spdlog::info("Got ImGui IO");
    ImFont *font = io.Fonts->AddFontFromMemoryCompressedTTF(DROID_compressed_data, DROID_compressed_size, 18.0);
    if (font == nullptr) {
        spdlog::error("Failed to load Droid Sans.");
    }
    spdlog::info("Loaded Droid Sans font");

    ImGui::StyleColorsDark();
    ImGuiStyle &st = ImGui::GetStyle();
    setupStyle(st);


    if (m_firstFrame) {
        m_firstFrame = false;

        spdlog::info("Starting game data initialization thread");

        // Game specific initialization stuff
        std::thread initThread([this]() {
            m_globals = std::make_unique<REGlobals>();
            m_mods = std::make_unique<Mods>();

            auto e = m_mods->onInitialize();

            if (e) {
                if (e->empty()) {
                    m_error = "An unknown error has occurred.";
                } else {
                    m_error = *e;
                }
            }

            m_gameDataInitialized = true;
        });

        initThread.detach();
    }

    return true;
}

void REFramework::createRenderTarget() {
    cleanupRenderTarget();

    ID3D11Texture2D *backBuffer{nullptr};
    if (m_d3d11Hook->getSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *) &backBuffer) == S_OK) {
        m_d3d11Hook->getDevice()->CreateRenderTargetView(backBuffer, NULL, &m_mainRenderTargetView);
        backBuffer->Release();
    }
}

void REFramework::cleanupRenderTarget() {
    if (m_mainRenderTargetView != nullptr) {
        m_mainRenderTargetView->Release();
        m_mainRenderTargetView = nullptr;
    }
}

const static void setupStyle(ImGuiStyle &st) {
    st.FrameBorderSize = 1.0f;
    st.FramePadding = ImVec2(4.0f, 2.0f);
    st.ItemSpacing = ImVec2(8.0f, 2.0f);
    st.WindowBorderSize = 1.0f;
    st.TabBorderSize = 1.0f;
    st.WindowRounding = 1.0f;
    st.ChildRounding = 1.0f;
    st.FrameRounding = 1.0f;
    st.ScrollbarRounding = 1.0f;
    st.GrabRounding = 1.0f;
    st.TabRounding = 1.0f;

    ImVec4 *colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.53f, 0.53f, 0.53f, 0.46f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.85f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 0.53f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.79f, 0.79f, 0.79f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.48f, 0.47f, 0.47f, 0.91f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.55f, 0.55f, 0.62f);
    colors[ImGuiCol_Button] = ImVec4(0.50f, 0.50f, 0.50f, 0.63f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.67f, 0.67f, 0.68f, 0.63f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.63f);
    colors[ImGuiCol_Header] = ImVec4(0.54f, 0.54f, 0.54f, 0.58f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.65f, 0.65f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f);
    colors[ImGuiCol_Separator] = ImVec4(0.58f, 0.58f, 0.58f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 0.64f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.87f, 0.87f, 0.87f, 0.53f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.87f, 0.87f, 0.87f, 0.74f);
    colors[ImGuiCol_Tab] = ImVec4(0.01f, 0.01f, 0.01f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.68f, 0.68f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.77f, 0.33f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.87f, 0.55f, 0.08f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.47f, 0.60f, 0.76f, 0.47f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
