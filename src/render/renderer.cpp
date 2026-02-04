#include "renderer.h"

#include "stdexcept"
#include "vector"

#include "glad/glad.h"

#include "render/geometry_gen.h"
#include "render/shader_utils.h"

Renderer::Renderer() = default;

Renderer::~Renderer()
{
    destroy();
}

void Renderer::init()
{
    m_lineProg.create();
    m_gridMesh.upload(geometry_gen::generateGrid());
    m_cubeWireMesh.upload(geometry_gen::generateCubeWire());

    createSolidShader();
    generateCubeSolidMesh();
}

void Renderer::destroy()
{
    // Mesh/line は各destroyで安全に二重呼びOKにしておく
    m_gridMesh.destroy();
    m_cubeWireMesh.destroy();
    m_lineProg.destroy();

    if (m_cubeSolidVBO) { glDeleteBuffers(1, &m_cubeSolidVBO); m_cubeSolidVBO = 0; }
    if (m_cubeSolidVAO) { glDeleteVertexArrays(1, &m_cubeSolidVAO); m_cubeSolidVAO = 0; }

    if (m_solidProg) { glDeleteProgram(m_solidProg); m_solidProg = 0; }
    m_solidLocMVP = -1;
    m_solidLocColor = -1;
}

void Renderer::draw(const Mat4& vp, int w, int h, uint32_t selectedFace)
{
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // まず線（グリッド・ワイヤ）
    glUseProgram(m_lineProg.m_prog);
    glUniformMatrix4fv(m_lineProg.m_locMVP, 1, GL_FALSE, vp.m);

    m_cubeWireMesh.draw();
    m_gridMesh.draw();

    glUseProgram(0);

    // 次に選択面ハイライト
    drawSelectedFaceFill(vp, selectedFace);
}

void Renderer::createSolidShader()
{
    const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";
    const char* fs = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main(){ FragColor = uColor; }
)";

    GLuint sVS = shader_utils::CompileShader(GL_VERTEX_SHADER, vs);
    GLuint sFS = shader_utils::CompileShader(GL_FRAGMENT_SHADER, fs);
    if (!sVS || !sFS) throw std::runtime_error("Solid shader compile failed");

    m_solidProg = shader_utils::LinkProgram(sVS, sFS);
    glDeleteShader(sVS);
    glDeleteShader(sFS);
    if (!m_solidProg) throw std::runtime_error("Solid program link failed");

    m_solidLocMVP = glGetUniformLocation(m_solidProg, "uMVP");
    m_solidLocColor = glGetUniformLocation(m_solidProg, "uColor");
    if (m_solidLocMVP < 0 || m_solidLocColor < 0)
        throw std::runtime_error("Solid uniform not found");
}

void Renderer::generateCubeSolidMesh()
{
    auto pos = geometry_gen::generateCubeSolidPositions(0.5f);

    glGenVertexArrays(1, &m_cubeSolidVAO);
    glGenBuffers(1, &m_cubeSolidVBO);

    glBindVertexArray(m_cubeSolidVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeSolidVBO);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(Vec3), pos.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::drawSelectedFaceFill(const Mat4& vp, uint32_t selectedFace)
{
    if (selectedFace == 0) return;

    const uint32_t face = selectedFace; // 1..6
    const GLint first = (GLint)((face - 1) * 6);
    const GLsizei count = 6;

    // 深度は有効のまま
    glEnable(GL_DEPTH_TEST);

    // Z-fighting対策
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    // 透明
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_solidProg);
    glUniformMatrix4fv(m_solidLocMVP, 1, GL_FALSE, vp.m);
    glUniform4f(m_solidLocColor, 1.0f, 0.8f, 0.2f, 0.25f);

    glBindVertexArray(m_cubeSolidVAO);
    glDrawArrays(GL_TRIANGLES, first, count);
    glBindVertexArray(0);

    glUseProgram(0);

    glDisable(GL_POLYGON_OFFSET_FILL);
}