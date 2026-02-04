#pragma once

#include "memory"

#include "GLFW/glfw3.h"

struct GlfwWindowDeleter { void operator()(GLFWwindow* p) const noexcept { if (p) glfwDestroyWindow(p); } };
using UniqueGlfwWindow = std::unique_ptr<GLFWwindow, GlfwWindowDeleter>;