#include "platform/input.h"

// この cpp 内だけで使うヘルパ
static InputState* GetInput(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    InputState* in = GetInput(window);
    if (!in) return;

    const bool down = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT)   in->m_leftDown = down;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) in->m_middleDown = down;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)  in->m_rightDown = down;
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    InputState* in = GetInput(window);
    if (!in) return;

    in->m_deltaX += (xpos - in->m_mouseX);
    in->m_deltaY += (ypos - in->m_mouseY);
    in->m_mouseX = xpos;
    in->m_mouseY = ypos;
}

static void ScrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset)
{
    InputState* in = GetInput(window);
    if (!in) return;

    in->m_scrollY += yoffset;
}

void Input::InstallInputCallbacks(GLFWwindow* window, InputState* input)
{
    // InputState の実体は Platform が持つ前提
    glfwSetWindowUserPointer(window, input);

    // GLFW のコールバックに接続
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);
}
