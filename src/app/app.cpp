#include "app.h"

#include "stdexcept"
#include "memory"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "math/math.h"
#include "platform/input.h"

App::App()
{
    // Platform ctor で window + context current

    // GL loader
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize glad");

    setGLState();

    // ImGui
    m_imgui = std::make_unique<ImGuiContextGuard>(
        m_platform.window(), "#version 330");

    // Renderer
    m_renderer.init();

    // Picker（Renderer依存）
    m_picker.init(
        m_platform.window(),
        m_renderer.cubeSolidVAO()
    );
}

void App::run()
{
    while (!m_platform.shouldClose())
    {
        // ---- 1) input ----
        m_platform.beginFrame();

        // ---- 2) ImGui begin ----
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ---- 3) update ----
        updateCameraFromInput();
        m_picker.updateRequest();

        // ---- 4) compute matrices ----
        int fbW = 0, fbH = 0;
        m_platform.framebufferSize(fbW, fbH);
        Mat4 vp = computeVP(fbW, fbH);

        // ---- 5) picking ----
        if (m_picker.hasRequest())
            m_selectedFace = m_picker.pick(vp, fbW, fbH);

        // ---- 6) UI ----
        drawUI();

        // ---- 7) render ----
        ImGui::Render();

        m_renderer.draw(vp, fbW, fbH, m_selectedFace);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_platform.window());
    }
}

void App::setGLState()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // sRGB（今は保留でもOK）
    glEnable(GL_FRAMEBUFFER_SRGB);
}

void App::updateCameraFromInput()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    const InputState& in = m_platform.input();

    if (in.m_leftDown)
        m_camera.orbit((float)in.m_deltaX, (float)in.m_deltaY);

    if (in.m_middleDown)
    {
        int w = 0, h = 0;
        m_platform.framebufferSize(w, h);
        m_camera.pan((float)in.m_deltaX, (float)in.m_deltaY, w, h);
    }

    m_camera.zoom((float)in.m_scrollY);
}

Mat4 App::computeVP(int fbW, int fbH) const
{
    const float aspect = (fbH > 0) ? (float)fbW / (float)fbH : 1.0f;
    Mat4 view = m_camera.viewMatrix();
    Mat4 proj = PerspectiveRH(60.0f * 3.1415926f / 180.0f, aspect, 0.1f, 1000.0f);
    return Mul(proj, view);
}

void App::drawUI()
{
    ImGui::Begin("Debug");
    ImGui::Text("Selected Face: %u", m_selectedFace);
    ImGui::Text("Yaw: %.3f  Pitch: %.3f  Dist: %.3f", m_camera.yaw(), m_camera.pitch(), m_camera.distance());
    ImGui::End();
}
