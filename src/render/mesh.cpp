#include "mesh.h"

Mesh::~Mesh()
{
	destroy();
}

void Mesh::upload(
    const std::vector<Vertex>& verts,
    const std::vector<uint32_t>& indices)
{
    destroy();

    m_indexCount = (GLsizei)indices.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        verts.size() * sizeof(Vertex),
        verts.data(),
        GL_STATIC_DRAW
    );

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint32_t),
        indices.data(),
        GL_STATIC_DRAW
    );

    // layout(location = 0) position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, position)
    );

    // layout(location = 1) color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, color)
    );

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(m_vao);
    glDrawElements(
        GL_TRIANGLES,
        m_indexCount,
        GL_UNSIGNED_INT,
        nullptr
    );
    glBindVertexArray(0);
}


void Mesh::destroy()
{
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);

    m_vao = m_vbo = m_ebo = 0;
    m_indexCount = 0;
}