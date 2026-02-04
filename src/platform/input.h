#pragma once

#include "GLFW/glfw3.h"

// 1フレーム分の入力状態
struct InputState
{
    bool m_leftDown = false;
    bool m_middleDown = false;
    bool m_rightDown = false;

    double m_mouseX = 0.0;
    double m_mouseY = 0.0;

    double m_deltaX = 0.0;
    double m_deltaY = 0.0;

    double m_scrollY = 0.0;

    void beginFrame()
    {
        m_deltaX = 0.0;
        m_deltaY = 0.0;
        m_scrollY = 0.0;
    }
};

// window に InputState* を紐付け＆コールバック登録
namespace Input
{
    void InstallInputCallbacks(GLFWwindow* window, InputState* input);
}