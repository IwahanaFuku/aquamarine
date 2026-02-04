#pragma once

#include "GLFW/glfw3.h"

class ImGuiContextGuard
{
public:
    ImGuiContextGuard(GLFWwindow* window, const char* glsl);
    ~ImGuiContextGuard();

    ImGuiContextGuard(const ImGuiContextGuard&) = delete;
    ImGuiContextGuard& operator=(const ImGuiContextGuard&) = delete;
};