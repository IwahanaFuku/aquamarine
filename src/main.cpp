#include <cstdio>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// -----------------------------
// Shader helpers
// -----------------------------
static GLuint CompileShader(GLenum type, const char* src)
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

static GLuint LinkProgram(GLuint vs, GLuint fs)
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

// -----------------------------
// Math (Vec3/Mat4)
// -----------------------------
struct Vec3
{
    float x, y, z;
    Vec3 operator+(const Vec3& r) const { return { x + r.x, y + r.y, z + r.z }; }
    Vec3 operator-(const Vec3& r) const { return { x - r.x, y - r.y, z - r.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};

static float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
static Vec3 Normalize(const Vec3& v)
{
    const float len2 = Dot(v, v);
    if (len2 <= 0.0f) return { 0,0,0 };
    const float invLen = 1.0f / std::sqrt(len2);
    return { v.x * invLen, v.y * invLen, v.z * invLen };
}

struct Mat4
{
    float m[16]{};
    static Mat4 Identity()
    {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
};

static Mat4 Mul(const Mat4& a, const Mat4& b)
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int r0 = 0; r0 < 4; ++r0)
            r.m[c * 4 + r0] =
            a.m[0 * 4 + r0] * b.m[c * 4 + 0] +
            a.m[1 * 4 + r0] * b.m[c * 4 + 1] +
            a.m[2 * 4 + r0] * b.m[c * 4 + 2] +
            a.m[3 * 4 + r0] * b.m[c * 4 + 3];
    return r;
}

static Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    Vec3 f = Normalize(center - eye);
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);

    Mat4 r = Mat4::Identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -Dot(s, eye);
    r.m[13] = -Dot(u, eye);
    r.m[14] = Dot(f, eye);
    return r;
}

static Mat4 PerspectiveRH(float fovY, float aspect, float zNear, float zFar)
{
    const float f = 1.0f / std::tan(fovY * 0.5f);
    Mat4 r{};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return r;
}

// -----------------------------
// InputState
// -----------------------------
struct InputState
{
    bool m_leftDown = false, m_middleDown = false, m_rightDown = false;
    double m_mouseX = 0.0, m_mouseY = 0.0;
    double m_deltaX = 0.0, m_deltaY = 0.0;
    double m_scrollY = 0.0;

    void beginFrame()
    {
        m_deltaX = 0.0;
        m_deltaY = 0.0;
        m_scrollY = 0.0;
    }
};

static InputState* GetInput(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    InputState* in = GetInput(window);
    if (!in) return;
    const bool down = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT)   in->m_leftDown = down;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) in->m_middleDown = down;
    if (button == GLFW_MOUSE_BUTTON_RIGHT)  in->m_rightDown = down;
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    InputState* in = GetInput(window);
    if (!in) return;

    in->m_deltaX += (xpos - in->m_mouseX);
    in->m_deltaY += (ypos - in->m_mouseY);
    in->m_mouseX = xpos;
    in->m_mouseY = ypos;
}

static void ScrollCallback(GLFWwindow* window, double, double yoffset)
{
    InputState* in = GetInput(window);
    if (!in) return;
    in->m_scrollY += yoffset;
}

// -----------------------------
// OrbitCamera
// -----------------------------
struct OrbitCamera
{
    float m_yaw = 0.0f, m_pitch = 0.0f, m_distance = 5.0f;
    float m_targetX = 0.0f, m_targetY = 0.0f, m_targetZ = 0.0f;

    void orbit(float dx, float dy)
    {
        const float k = 0.001f;
        m_yaw -= dx * k;
        m_pitch += dy * k;
        const float lim = 1.553343f;
        if (m_pitch > lim) m_pitch = lim;
        if (m_pitch < -lim) m_pitch = -lim;
    }

    void zoom(float wheel)
    {
        const float k = 0.9f;
        if (wheel > 0) m_distance *= k;
        if (wheel < 0) m_distance /= k;
        if (m_distance < 0.1f) m_distance = 0.1f;
        if (m_distance > 200.f) m_distance = 200.f;
    }

    void panXY(float dx, float dy) // 暫定：ワールドXY
    {
        const float k = 0.01f * m_distance;
        m_targetX += -dx * k;
        m_targetY += dy * k;
    }

    Vec3 target() const { return { m_targetX, m_targetY, m_targetZ }; }

    Vec3 eye() const
    {
        const float cy = std::cos(m_yaw);
        const float sy = std::sin(m_yaw);
        const float cp = std::cos(m_pitch);
        const float sp = std::sin(m_pitch);
        Vec3 dir = { cp * sy, sp, cp * cy };
        return target() + dir * m_distance;
    }

    Mat4 view() const
    {
        return LookAtRH(eye(), target(), { 0,1,0 });
    }
};

// -----------------------------
// GL resources
// -----------------------------
struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

class LineProgram
{
public:
    GLuint m_prog = 0;
    GLint  m_locMVP = -1;

    void create()
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

        GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
        if (!vs || !fs) throw std::runtime_error("Shader compile failed");

        m_prog = LinkProgram(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        if (!m_prog) throw std::runtime_error("Program link failed");

        m_locMVP = glGetUniformLocation(m_prog, "uMVP");
        if (m_locMVP < 0) throw std::runtime_error("uMVP not found");
    }

    void destroy()
    {
        if (m_prog) glDeleteProgram(m_prog);
        m_prog = 0;
        m_locMVP = -1;
    }

    ~LineProgram() { destroy(); }
};

class LineMesh
{
public:
    GLuint m_vao = 0, m_vbo = 0;
    GLsizei m_count = 0;

    void upload(const std::vector<Vertex>& verts)
    {
        destroy();

        m_count = (GLsizei)verts.size();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw() const
    {
        glBindVertexArray(m_vao);
        glDrawArrays(GL_LINES, 0, m_count);
        glBindVertexArray(0);
    }

    void destroy()
    {
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        m_vao = 0; m_vbo = 0; m_count = 0;
    }

    ~LineMesh() { destroy(); }
};

// -----------------------------
// CPU geometry generation
// -----------------------------
static std::vector<Vertex> CreateGrid(int half = 10, float step = 1.0f)
{
    std::vector<Vertex> grid;
    grid.reserve((size_t)((half * 2 + 1) * 4));

    const float y = 0.0f;
    const Vertex colMinor = { 0,0,0, 0.35f,0.35f,0.38f,0.6f };
    const Vertex colMajor = { 0,0,0, 0.55f,0.55f,0.60f,0.9f };
    const Vertex colAxisX = { 0,0,0, 0.90f,0.20f,0.20f,1.0f };
    const Vertex colAxisZ = { 0,0,0, 0.20f,0.35f,0.90f,1.0f };

    auto pushLine = [&](float x0, float y0, float z0, float x1, float y1, float z1, const Vertex& c)
        {
            grid.push_back({ x0,y0,z0, c.r,c.g,c.b,c.a });
            grid.push_back({ x1,y1,z1, c.r,c.g,c.b,c.a });
        };

    for (int i = -half; i <= half; ++i)
    {
        const float v = i * step;
        const bool isAxis = (i == 0);
        const bool isMajor = (i % 5 == 0);

        Vertex c = isMajor ? colMajor : colMinor;

        pushLine(-half * step, y, v, half * step, y, v, isAxis ? colAxisZ : c);
        pushLine(v, y, -half * step, v, y, half * step, isAxis ? colAxisX : c);
    }
    return grid;
}

static std::vector<Vertex> CreateCubeWire(float s = 0.5f)
{
    std::vector<Vertex> cube;
    cube.reserve(24);

    auto addEdge = [&](float x0, float y0, float z0, float x1, float y1, float z1)
        {
            const float r = 0.95f, g = 0.85f, b = 0.35f, a = 1.0f;
            cube.push_back({ x0,y0,z0,r,g,b,a });
            cube.push_back({ x1,y1,z1,r,g,b,a });
        };

    // bottom
    addEdge(-s, -s, -s, s, -s, -s);
    addEdge(s, -s, -s, s, -s, s);
    addEdge(s, -s, s, -s, -s, s);
    addEdge(-s, -s, s, -s, -s, -s);
    // top
    addEdge(-s, s, -s, s, s, -s);
    addEdge(s, s, -s, s, s, s);
    addEdge(s, s, s, -s, s, s);
    addEdge(-s, s, s, -s, s, -s);
    // vertical
    addEdge(-s, -s, -s, -s, s, -s);
    addEdge(s, -s, -s, s, s, -s);
    addEdge(s, -s, s, s, s, s);
    addEdge(-s, -s, s, -s, s, s);

    return cube;
}

// -----------------------------
// GLFW/ImGui RAII (reuse your existing ones)
// -----------------------------
static void GlfwErrorCallback(int error, const char* description)
{
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

class GlfwSystem
{
public:
    GlfwSystem()
    {
        glfwSetErrorCallback(GlfwErrorCallback);
        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");
    }
    ~GlfwSystem() { glfwTerminate(); }
    GlfwSystem(const GlfwSystem&) = delete;
    GlfwSystem& operator=(const GlfwSystem&) = delete;
};

struct GlfwWindowDeleter { void operator()(GLFWwindow* p) const noexcept { if (p) glfwDestroyWindow(p); } };
using UniqueGlfwWindow = std::unique_ptr<GLFWwindow, GlfwWindowDeleter>;

class ImGuiContextGuard
{
public:
    ImGuiContextGuard(GLFWwindow* window, const char* glsl)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl);
    }
    ~ImGuiContextGuard()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    ImGuiContextGuard(const ImGuiContextGuard&) = delete;
    ImGuiContextGuard& operator=(const ImGuiContextGuard&) = delete;
};

// -----------------------------
// App
// -----------------------------
class App
{
    GlfwSystem m_glfw;
    UniqueGlfwWindow m_window;
    std::unique_ptr<ImGuiContextGuard> m_imgui;

    InputState  m_input;
    OrbitCamera m_camera;

    LineProgram m_lineProg;
    LineMesh    m_gridMesh;
    LineMesh    m_cubeMesh;

public:
    App()
    {
        setWindowHints();
        m_window = createWindow(1280, 720, "MyModeler");
        glfwMakeContextCurrent(m_window.get());
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize glad");

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_imgui = std::make_unique<ImGuiContextGuard>(m_window.get(), "#version 330");

        setCallbacks();

        m_lineProg.create();
        m_gridMesh.upload(CreateGrid());
        m_cubeMesh.upload(CreateCubeWire());
    }

    void run()
    {
        while (!glfwWindowShouldClose(m_window.get()))
        {
            // 1) Input
            m_input.beginFrame();
            glfwPollEvents();

            // 2) ImGui frame begin
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 3) Update (camera)
            updateCameraFromInput();

            // 4) UI
            drawUI();

            // 5) Render (3D -> ImGui -> swap)
            renderFrame();
        }
    }

private:
    static void setWindowHints()
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    static UniqueGlfwWindow createWindow(int w, int h, const char* title)
    {
        GLFWwindow* raw = glfwCreateWindow(w, h, title, nullptr, nullptr);
        if (!raw) throw std::runtime_error("Failed to create GLFW window");
        return UniqueGlfwWindow(raw);
    }

    void setCallbacks()
    {
        glfwSetWindowUserPointer(m_window.get(), &m_input);
        glfwSetMouseButtonCallback(m_window.get(), MouseButtonCallback);
        glfwSetCursorPosCallback(m_window.get(), CursorPosCallback);
        glfwSetScrollCallback(m_window.get(), ScrollCallback);
    }

    void updateCameraFromInput()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        if (m_input.m_leftDown)
            m_camera.orbit((float)m_input.m_deltaX, (float)m_input.m_deltaY);

        if (m_input.m_middleDown)
            m_camera.panXY((float)m_input.m_deltaX, (float)m_input.m_deltaY);

        m_camera.zoom((float)m_input.m_scrollY);
    }

    Mat4 computeVP(int fbW, int fbH) const
    {
        const float aspect = (fbH > 0) ? (float)fbW / (float)fbH : 1.0f;
        Mat4 view = m_camera.view();
        Mat4 proj = PerspectiveRH(60.0f * 3.1415926f / 180.0f, aspect, 0.1f, 1000.0f);
        return Mul(proj, view);
    }

    void drawUI()
    {
        ImGui::Begin("Camera Debug");
        ImGui::Text("Yaw   : %.3f", m_camera.m_yaw);
        ImGui::Text("Pitch : %.3f", m_camera.m_pitch);
        ImGui::Text("Dist  : %.3f", m_camera.m_distance);
        ImGui::Text("Target: %.2f %.2f %.2f", m_camera.m_targetX, m_camera.m_targetY, m_camera.m_targetZ);
        ImGui::End();
    }

    void renderFrame()
    {
        ImGui::Render();

        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Mat4 vp = computeVP(w, h);

        glUseProgram(m_lineProg.m_prog);
        glUniformMatrix4fv(m_lineProg.m_locMVP, 1, GL_FALSE, vp.m);

        m_gridMesh.draw();
        m_cubeMesh.draw();

        glUseProgram(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window.get());
    }
};

int main()
{
    try
    {
        App app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
