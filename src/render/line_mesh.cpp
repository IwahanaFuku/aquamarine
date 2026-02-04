#include "line_mesh.h"

#include "vector"

LineMesh::~LineMesh()
{
    destroy();
}

void LineMesh::upload(const std::vector<Vertex>& verts)
{
    destroy();

    m_count = (GLsizei)verts.size();
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void LineMesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINES, 0, m_count);
    glBindVertexArray(0);
}

void LineMesh::destroy()
{
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_vao = 0; m_vbo = 0; m_count = 0;
}
