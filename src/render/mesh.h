#pragma once

#include "vector"

#include "glad/glad.h"

#include "render/vertex.h"

class Mesh
{
public:
	GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
	GLsizei m_indexCount = 0;

	~Mesh();

	void upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
	void draw() const;
	void destroy();
};