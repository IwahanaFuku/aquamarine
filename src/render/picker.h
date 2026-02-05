#pragma once

#include "stdexcept"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

class Picker
{
public:
    Picker();
    ~Picker();

    void init(GLFWwindow* window, GLuint cubeSolidVAO);
    bool isReady() const;
    void updateRequest();
    bool hasRequest() const;
    uint32_t pick(const glm::mat4& vp, int fbW, int fbH);
    void destroy();

    Picker(const Picker&) = delete;
    Picker& operator=(const Picker&) = delete;

private:
    GLFWwindow* m_window = nullptr;
    GLuint m_cubeSolidVAO = 0;

    GLuint m_FBO = 0;
    GLuint m_tex = 0;
    GLuint m_depth = 0;
    int m_pickW = 0, m_pickH = 0;

    GLuint m_prog = 0;
    GLint  m_locMVP = -1;
    GLint  m_locID = -1;

    bool   m_pickRequested = false;
    double m_pickX = 0.0, m_pickY = 0.0;

    void createShader();

    void ensureFBO(int w, int h);
    uint32_t doPicking(const glm::mat4& view, int fbW, int fbH, double mouseX, double mouseY);
};

