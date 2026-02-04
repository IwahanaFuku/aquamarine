#include "shader_utils.h"

#include "vector"

GLuint shader_utils::CompileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log((size_t)len);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error: %s\n", log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint shader_utils::LinkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log((size_t)len);
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link error: %s\n", log.data());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}