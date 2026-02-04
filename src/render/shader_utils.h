#pragma once

#include "glad/glad.h"

namespace shader_utils
{
    GLuint CompileShader(GLenum type, const char* src);
    GLuint LinkProgram(GLuint vs, GLuint fs);
}
