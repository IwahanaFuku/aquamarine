#include "picker.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

#include "render/shader_utils.h"

Picker::Picker() = default;

Picker::~Picker()
{
    destroy();
}

void Picker::init(GLFWwindow* window, GLuint cubeSolidVAO)
{
    m_window = window;
    m_cubeSolidVAO = cubeSolidVAO;
    createShader();
}

bool Picker::isReady() const
{
    return m_prog != 0;
}

void Picker::updateRequest()
{
    if (!m_window) return;

    // ImGuiがマウスを使ってるなら無視
    if (ImGui::GetIO().WantCaptureMouse) return;

    static int prev = GLFW_RELEASE;
    int now = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);

    if (now == GLFW_PRESS && prev == GLFW_RELEASE)
    {
        glfwGetCursorPos(m_window, &m_pickX, &m_pickY);
        m_pickRequested = true;
    }
    prev = now;
}

bool Picker::hasRequest() const
{
    return m_pickRequested;
}

uint32_t Picker::pick(const glm::mat4& vp, int fbW, int fbH)
{
    if (!m_pickRequested) return 0;

    m_pickRequested = false;
    return doPicking(vp, fbW, fbH, m_pickX, m_pickY);
}

void Picker::destroy()
{
    if (m_depth) { glDeleteRenderbuffers(1, &m_depth); m_depth = 0; }
    if (m_tex) { glDeleteTextures(1, &m_tex); m_tex = 0; }
    if (m_FBO) { glDeleteFramebuffers(1, &m_FBO); m_FBO = 0; }

    if (m_prog) { glDeleteProgram(m_prog); m_prog = 0; }
    m_locMVP = -1;
    m_locID = -1;

    m_pickW = 0; m_pickH = 0;
}

void Picker::createShader()
{
    m_prog = shader_utils::BuildProgramFromGLSLFile("assets/shaders/pick.glsl");
    m_locMVP = shader_utils::GetUniformOrThrow(m_prog, "uMVP");
    m_locID = shader_utils::GetUniformOrThrow(m_prog, "uID");
}

void Picker::ensureFBO(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (m_FBO && w == m_pickW && h == m_pickH) return;

    if (m_depth) { glDeleteRenderbuffers(1, &m_depth); m_depth = 0; }
    if (m_tex) { glDeleteTextures(1, &m_tex); m_tex = 0; }
    if (m_FBO) { glDeleteFramebuffers(1, &m_FBO); m_FBO = 0; }

    m_pickW = w; m_pickH = h;

    glGenFramebuffers(1, &m_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);

    glGenRenderbuffers(1, &m_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Pick FBO is not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t Picker::doPicking(const glm::mat4& view, int fbW, int fbH, double mouseX, double mouseY)
{
    ensureFBO(fbW, fbH);

    // 範囲外ガード（DPI/ウィンドウ外クリック対策）
    int px = (int)mouseX;
    int py = fbH - 1 - (int)mouseY;
    if (px < 0 || px >= fbW || py < 0 || py >= fbH)
        return 0;

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, fbW, fbH);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    GLuint clearID = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearID);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_prog);
    glUniformMatrix4fv(m_locMVP, 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(m_cubeSolidVAO);

    for (uint32_t face = 0; face < 6; ++face)
    {
        glUniform1ui(m_locID, face + 1);
        glDrawArrays(GL_TRIANGLES, (GLint)(face * 6), 6);
    }

    glBindVertexArray(0);
    glUseProgram(0);

    uint32_t out = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(px, py, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &out);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 通常描画に戻す（最低限）
    glEnable(GL_BLEND);

    return out;
}
