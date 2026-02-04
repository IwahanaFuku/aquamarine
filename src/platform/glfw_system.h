#pragma once

#include "stdexcept"

#include "GLFW/glfw3.h"


static void GlfwErrorCallback(int error, const char* description)
{
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

class GlfwSystem
{
public:
    GlfwSystem();

    ~GlfwSystem() { glfwTerminate(); }
    GlfwSystem(const GlfwSystem&) = delete;
    GlfwSystem& operator=(const GlfwSystem&) = delete;
};
