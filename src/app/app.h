#pragma once

#include "stdexcept"
#include "memory"

#include "glm/glm.hpp"

#include "camera/orbit_camera.h"
#include "platform/imgui_context_guard.h"
#include "platform/platform.h"
#include "render/renderer.h"
#include "render/picker.h"

class App
{
public:

    App();

    void run();

private:
    Platform m_platform;
    std::unique_ptr<ImGuiContextGuard> m_imgui;

    Renderer m_renderer;
    Picker   m_picker;

    OrbitCamera m_camera;
    uint32_t m_selectedFace = 0;
    static void setGLState();
    void updateCameraFromInput();
    glm::mat4 computeVP(int fbW, int fbH) const;
    void drawUI();
};