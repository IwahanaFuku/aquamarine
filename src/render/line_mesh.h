#pragma once

#include "vector"

#include "glad/glad.h"

#include "render/vertex.h"

class LineMesh
{
public:
    GLuint m_vao = 0, m_vbo = 0;
    GLsizei m_count = 0;

    ~LineMesh();

    void upload(const std::vector<Vertex>& verts);
    void draw() const;
    void destroy();
};