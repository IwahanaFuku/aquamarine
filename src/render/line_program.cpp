#include "line_program.h"

#include "stdexcept"

#include "shader_utils.h"

LineProgram::~LineProgram()
{
    destroy();
}

void LineProgram::create()
{
    {
        const char* vsSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main(){ vColor=aColor; gl_Position = uMVP * vec4(aPos,1.0); }
)";
        const char* fsSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main(){ FragColor=vColor; }
)";

        GLuint vs = shader_utils::CompileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = shader_utils::CompileShader(GL_FRAGMENT_SHADER, fsSrc);
        if (!vs || !fs) throw std::runtime_error("Shader compile failed");

        m_prog = shader_utils::LinkProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        if (!m_prog) throw std::runtime_error("Program link failed");

        m_locMVP = glGetUniformLocation(m_prog, "uMVP");
        if (m_locMVP < 0) throw std::runtime_error("uMVP not found");
    }
}

void LineProgram::destroy()
{
    if (m_prog) glDeleteProgram(m_prog);
    m_prog = 0;
    m_locMVP = -1;
}
