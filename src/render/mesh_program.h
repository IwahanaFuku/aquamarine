#pragma once

#include "glad/glad.h"

class MeshProgram
{
public:
    GLuint m_prog = 0;
    GLint  m_locMVP = -1, m_locColor = -1;

    ~MeshProgram();

    void create();
    void destroy();
};