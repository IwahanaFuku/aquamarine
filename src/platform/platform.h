#pragma once

#include "platform/glfw_system.h"
#include "platform/input.h"
#include "platform/window.h"

class Platform
{
public:
    Platform();
    ~Platform();

    void beginFrame();
    bool shouldClose() const;

    GLFWwindow* window() const { return m_window.get(); }
    InputState& input() { return m_input; }
    const InputState& input() const { return m_input; }
    void framebufferSize(int& w, int& h) const { glfwGetFramebufferSize(m_window.get(), &w, &h); }
    void cursorPos(double& x, double& y) const { glfwGetCursorPos(m_window.get(), &x, &y); }

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;
    Platform(Platform&&) = delete;
    Platform& operator=(Platform&&) = delete;

private:
    GlfwSystem m_glfw;
    UniqueGlfwWindow m_window;
    InputState m_input;

    static void setWindowHints()
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    }

    static UniqueGlfwWindow createWindow(int w, int h, const char* title)
    {
        GLFWwindow* raw = glfwCreateWindow(w, h, title, nullptr, nullptr);
        if (!raw) throw std::runtime_error("Failed to create GLFW window");
        return UniqueGlfwWindow(raw);
    }
};