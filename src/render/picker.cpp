#include "picker.h"

#include "imgui.h"

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
    createPickShader();
}

bool Picker::isReady() const
{
    return m_pickProg != 0;
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

uint32_t Picker::pick(const Mat4& vp, int fbW, int fbH)
{
    if (!m_pickRequested) return 0;

    m_pickRequested = false;
    return doPicking(vp, fbW, fbH, m_pickX, m_pickY);
}

void Picker::destroy()
{
    if (m_pickDepth) { glDeleteRenderbuffers(1, &m_pickDepth); m_pickDepth = 0; }
    if (m_pickTex) { glDeleteTextures(1, &m_pickTex); m_pickTex = 0; }
    if (m_pickFBO) { glDeleteFramebuffers(1, &m_pickFBO); m_pickFBO = 0; }

    if (m_pickProg) { glDeleteProgram(m_pickProg); m_pickProg = 0; }
    m_pickLocMVP = -1;
    m_pickLocID = -1;

    m_pickW = 0; m_pickH = 0;
}

void Picker::createPickShader()
{
    const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";
    const char* fs = R"(
#version 330 core
uniform uint uID;
layout(location=0) out uint outID;
void main(){ outID = uID; }
)";

    GLuint sVS = shader_utils::CompileShader(GL_VERTEX_SHADER, vs);
    GLuint sFS = shader_utils::CompileShader(GL_FRAGMENT_SHADER, fs);
    if (!sVS || !sFS) throw std::runtime_error("Pick shader compile failed");

    m_pickProg = shader_utils::LinkProgram(sVS, sFS);
    glDeleteShader(sVS);
    glDeleteShader(sFS);
    if (!m_pickProg) throw std::runtime_error("Pick program link failed");

    m_pickLocMVP = glGetUniformLocation(m_pickProg, "uMVP");
    m_pickLocID = glGetUniformLocation(m_pickProg, "uID");
    if (m_pickLocMVP < 0 || m_pickLocID < 0)
        throw std::runtime_error("Pick uniform not found");
}

void Picker::ensurePickFBO(int w, int h)
{
    if (w <= 0 || h <= 0) return;
    if (m_pickFBO && w == m_pickW && h == m_pickH) return;

    if (m_pickDepth) { glDeleteRenderbuffers(1, &m_pickDepth); m_pickDepth = 0; }
    if (m_pickTex) { glDeleteTextures(1, &m_pickTex); m_pickTex = 0; }
    if (m_pickFBO) { glDeleteFramebuffers(1, &m_pickFBO); m_pickFBO = 0; }

    m_pickW = w; m_pickH = h;

    glGenFramebuffers(1, &m_pickFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickFBO);

    glGenTextures(1, &m_pickTex);
    glBindTexture(GL_TEXTURE_2D, m_pickTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickTex, 0);

    glGenRenderbuffers(1, &m_pickDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_pickDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pickDepth);

    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Pick FBO is not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t Picker::doPicking(const Mat4& vp, int fbW, int fbH, double mouseX, double mouseY)
{
    ensurePickFBO(fbW, fbH);

    // 範囲外ガード（DPI/ウィンドウ外クリック対策）
    int px = (int)mouseX;
    int py = fbH - 1 - (int)mouseY;
    if (px < 0 || px >= fbW || py < 0 || py >= fbH)
        return 0;

    glBindFramebuffer(GL_FRAMEBUFFER, m_pickFBO);
    glViewport(0, 0, fbW, fbH);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    GLuint clearID = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearID);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_pickProg);
    glUniformMatrix4fv(m_pickLocMVP, 1, GL_FALSE, vp.m);

    glBindVertexArray(m_cubeSolidVAO);

    for (uint32_t face = 0; face < 6; ++face)
    {
        glUniform1ui(m_pickLocID, face + 1);
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
