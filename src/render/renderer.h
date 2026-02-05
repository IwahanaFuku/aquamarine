#pragma once

#include "glm/glm.hpp"

#include "render/line_mesh.h"
#include "render/line_program.h"
#include "render/mesh.h"
#include "render/mesh_program.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void init();
    void destroy();
    void draw(const glm::mat4& vp, int w, int h, uint32_t selectedFace);

    GLuint cubeSolidVAO() const { return m_cubeSolidVAO; }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

private:
    // --- Line ---
    LineProgram m_lineProg;
    LineMesh    m_gridMesh;
    LineMesh    m_cubeWireMesh;

    // --- Mesh ---
    MeshProgram m_meshProg;
    Mesh m_cubeMesh;

    // --- Solid highlight ---
    GLuint m_cubeSolidVAO = 0;
    GLuint m_cubeSolidVBO = 0;

    GLuint m_solidProg = 0;
    GLint  m_solidLocMVP = -1;
    GLint  m_solidLocColor = -1;

    void createSolidShader();
    void generateCubeSolidMesh();
    void drawSelectedFaceFill(const glm::mat4& vp, uint32_t selectedFace);
};