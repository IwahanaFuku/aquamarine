#pragma once

#include "glad/glad.h"
#include "string"
#include "string_view"

namespace shader_utils
{
    std::string ReadTextFile(const char* path);
    GLuint BuildProgramFromGLSLFile(const char* path);
    GLuint CompileShader(GLenum type, const char* src);
    GLuint LinkProgram(GLuint vs, GLuint fs);
    GLuint BuildProgramFromSource(
        std::string_view vsSrc,
        std::string_view fsSrc);
    GLint GetUniformOrThrow(GLuint program, const char* name);
}
