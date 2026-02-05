#include "mesh_program.h"

#include "stdexcept"

#include "shader_utils.h"

MeshProgram::~MeshProgram()
{
    destroy();
}

void MeshProgram::create()
{
    m_prog = shader_utils::BuildProgramFromGLSLFile("assets/shaders/solid.glsl");
    m_locMVP = shader_utils::GetUniformOrThrow(m_prog, "uMVP");
    m_locColor = shader_utils::GetUniformOrThrow(m_prog, "uColor");
}

void MeshProgram::destroy()
{
    if (m_prog) glDeleteProgram(m_prog);
    m_prog = 0;
    m_locMVP = -1;
    m_locColor = -1;
}
