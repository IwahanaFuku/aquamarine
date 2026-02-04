#include "glfw_system.h"

GlfwSystem::GlfwSystem()
{
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");
}