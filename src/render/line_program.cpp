#include "line_program.h"

#include "stdexcept"

#include "shader_utils.h"

LineProgram::~LineProgram()
{
    destroy();
}

void LineProgram::create()
{
    m_prog = shader_utils::BuildProgramFromGLSLFile("assets/shaders/line.glsl");
    m_locMVP = shader_utils::GetUniformOrThrow(m_prog, "uMVP");
}

void LineProgram::destroy()
{
    if (m_prog) glDeleteProgram(m_prog);
    m_prog = 0;
    m_locMVP = -1;
}
