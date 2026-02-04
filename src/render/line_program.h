#pragma once

#include "glad/glad.h"

class LineProgram
{
public:
    GLuint m_prog = 0;
    GLint  m_locMVP = -1;

    ~LineProgram();

    void create();
    void destroy();
};