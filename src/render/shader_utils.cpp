#include "shader_utils.h"

#include "vector"
#include "fstream"
#include "sstream"
#include "stdexcept"

std::string shader_utils::ReadTextFile(const char* path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error(std::string("Failed to open shader file: ") + path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

static std::string InjectDefineAfterVersion(const std::string& src, const char* defineLine)
{
    // "#version" 行の直後に "#define VERTEX/FRAGMENT" を挿入する
    // 例: "#version 330 core\n#define VERTEX\n..."
    const std::string versionTag = "#version";
    auto pos = src.find(versionTag);
    if (pos == std::string::npos)
    {
        // #version が無い場合は先頭に足す（動くが非推奨）
        return std::string(defineLine) + "\n" + src;
    }

    // #version 行末まで
    auto eol = src.find('\n', pos);
    if (eol == std::string::npos)
        throw std::runtime_error("Shader source has #version but no newline");

    std::string out;
    out.reserve(src.size() + 64);
    out.append(src.substr(0, eol + 1));
    out.append(defineLine);
    out.append("\n");
    out.append(src.substr(eol + 1));
    return out;
}

GLuint shader_utils::BuildProgramFromGLSLFile(const char* path)
{
    const std::string src = ReadTextFile(path);

    const std::string vsSrc = InjectDefineAfterVersion(src, "#define VERTEX 1");
    const std::string fsSrc = InjectDefineAfterVersion(src, "#define FRAGMENT 1");

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc.c_str());
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc.c_str());
    if (!vs || !fs) throw std::runtime_error("Shader compile failed");

    GLuint prog = LinkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) throw std::runtime_error("Program link failed");

    return prog;
}

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

GLuint shader_utils::BuildProgramFromSource(std::string_view vsSrc, std::string_view fsSrc)
{
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc.data());
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc.data());
    if (!vs || !fs) throw std::runtime_error("Shader compile failed");

    GLuint prog = LinkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) throw std::runtime_error("Program link failed");

    return prog;
}

GLint shader_utils::GetUniformOrThrow(GLuint program, const char* name)
{
    GLint loc = glGetUniformLocation(program, name);
    if (loc < 0) throw std::runtime_error(std::string("Uniform not found: ") + name);
    return loc;
}
