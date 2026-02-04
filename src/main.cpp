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

    void panCamera(float dxPixels, float dyPixels, int fbW, int fbH)
    {
        // 1) カメラ基底（right/up）を取得
        Mat4 viewMat= view();

        Vec3 right = { viewMat.m[0], viewMat.m[4], viewMat.m[8] }; // s
        Vec3 up = { viewMat.m[1], viewMat.m[5], viewMat.m[9] }; // u

        // 2) ピクセル差分 → ワールド移動量へスケール
        const float h = (fbH > 0) ? (float)fbH : 1.0f;

        // fovY は Projection と同じ値を入れておく
        const float fovY = 60.0f * 3.1415926f / 180.0f;
        const float worldPerPixel = (2.0f * m_distance * std::tan(fovY * 0.5f)) / h;

        // 3) target を right/up 方向へ動かす
        Vec3 t = target();

        t = t + right * (-dxPixels * worldPerPixel);
        t = t + up * (dyPixels * worldPerPixel);

        m_targetX = t.x;
        m_targetY = t.y;
        m_targetZ = t.z;
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

static std::vector<Vec3> CreateCubeSolidPositions(float s = 0.5f)
{
    // 6 faces * 2 triangles * 3 verts = 36
    std::vector<Vec3> v;
    v.reserve(36);

    auto tri = [&](Vec3 a, Vec3 b, Vec3 c) { v.push_back(a); v.push_back(b); v.push_back(c); };

    // corners
    Vec3 p000{ -s,-s,-s }, p001{ -s,-s, s }, p010{ -s, s,-s }, p011{ -s, s, s };
    Vec3 p100{ s,-s,-s }, p101{ s,-s, s }, p110{ s, s,-s }, p111{ s, s, s };

    // +X face (ID=1) : p100 p101 p111 p110
    tri(p100, p101, p111); tri(p100, p111, p110);
    // -X face (ID=2) : p000 p010 p011 p001
    tri(p000, p010, p011); tri(p000, p011, p001);
    // +Y face (ID=3) : p010 p110 p111 p011
    tri(p010, p110, p111); tri(p010, p111, p011);
    // -Y face (ID=4) : p000 p001 p101 p100
    tri(p000, p001, p101); tri(p000, p101, p100);
    // +Z face (ID=5) : p001 p011 p111 p101
    tri(p001, p011, p111); tri(p001, p111, p101);
    // -Z face (ID=6) : p000 p100 p110 p010
    tri(p000, p100, p110); tri(p000, p110, p010);

    return v;
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

    GLuint m_pickFBO = 0;
    GLuint m_pickTex = 0;   // GL_R32UI
    GLuint m_pickDepth = 0; // depth renderbuffer
    int m_pickW = 0, m_pickH = 0;

    GLuint m_pickProg = 0;
    GLint  m_pickLocMVP = -1;
    GLint  m_pickLocID = -1;

    GLuint m_cubeSolidVAO = 0;
    GLuint m_cubeSolidVBO = 0;

    uint32_t m_selectedFace = 0; // 0 = none
    bool m_pickRequested = false;
    double m_pickX = 0.0, m_pickY = 0.0;

    GLuint m_solidProg = 0;
    GLint  m_solidLocMVP = -1;
    GLint  m_solidLocColor = -1;

public:
    App()
    {
        setWindowHints();
        m_window = createWindow(1280, 720, "MyModeler");
        glfwMakeContextCurrent(m_window.get());
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize glad");

        setGLState();

        m_imgui = std::make_unique<ImGuiContextGuard>(m_window.get(), "#version 330");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        checksRGB();
        setCallbacks();

        createPickShader();
        createSolidShader();
        createCubeSolidMesh();

        m_lineProg.create();
        m_gridMesh.upload(CreateGrid());
        m_cubeMesh.upload(CreateCubeWire());
    }

    void checksRGB()
    {
        GLint enc = 0;
        glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER,
            GL_BACK_LEFT,
            GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
            &enc
        );

        if (enc == GL_SRGB)
            std::printf("Backbuffer encoding: sRGB\n");
        else
            std::printf("Backbuffer encoding: Linear (not sRGB)\n");
    }

    void setGLState()
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // sRGB
        glEnable(GL_FRAMEBUFFER_SRGB);
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
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
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
        {
            int w, h;
            glfwGetFramebufferSize(m_window.get(), &w, &h);
            m_camera.panCamera((float)m_input.m_deltaX, (float)m_input.m_deltaY, w, h);
        }

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
        ImGui::Text("Selected Face ID: %u", m_selectedFace);
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

        requestPickIfClicked();

        Mat4 vp = computeVP(w, h);

        if (m_pickRequested)
        {
            m_selectedFace = doPicking(vp, w, h, m_pickX, m_pickY);
            m_pickRequested = false;
        }

        glUseProgram(m_lineProg.m_prog);
        glUniformMatrix4fv(m_lineProg.m_locMVP, 1, GL_FALSE, vp.m);

        m_cubeMesh.draw();
        m_gridMesh.draw();
        drawSelectedFaceFill(vp);

        glUseProgram(0);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window.get());
    }

    void ensurePickFBO(int w, int h)
    {
        if (w <= 0 || h <= 0) return;
        if (m_pickFBO && w == m_pickW && h == m_pickH) return;

        // destroy old
        if (m_pickDepth) { glDeleteRenderbuffers(1, &m_pickDepth); m_pickDepth = 0; }
        if (m_pickTex) { glDeleteTextures(1, &m_pickTex); m_pickTex = 0; }
        if (m_pickFBO) { glDeleteFramebuffers(1, &m_pickFBO); m_pickFBO = 0; }

        m_pickW = w; m_pickH = h;

        glGenFramebuffers(1, &m_pickFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_pickFBO);

        // integer color attachment
        glGenTextures(1, &m_pickTex);
        glBindTexture(GL_TEXTURE_2D, m_pickTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, w, h, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickTex, 0);

        // depth
        glGenRenderbuffers(1, &m_pickDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, m_pickDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pickDepth);

        GLenum drawBuf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawBuf);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("Pick FBO is not complete");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void createPickShader()
    {
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";

        const char* fs = R"(
#version 330 core
uniform uint uID;
layout(location=0) out uint outID;
void main(){ outID = uID; }
)";

        GLuint sVS = CompileShader(GL_VERTEX_SHADER, vs);
        GLuint sFS = CompileShader(GL_FRAGMENT_SHADER, fs);
        if (!sVS || !sFS) throw std::runtime_error("Pick shader compile failed");

        m_pickProg = LinkProgram(sVS, sFS);
        glDeleteShader(sVS);
        glDeleteShader(sFS);
        if (!m_pickProg) throw std::runtime_error("Pick program link failed");

        m_pickLocMVP = glGetUniformLocation(m_pickProg, "uMVP");
        m_pickLocID = glGetUniformLocation(m_pickProg, "uID");
        if (m_pickLocMVP < 0 || m_pickLocID < 0)
            throw std::runtime_error("Pick uniform not found");
    }

    void createSolidShader()
    {
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos, 1.0); }
)";

        const char* fs = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main(){ FragColor = uColor; }
)";

        GLuint sVS = CompileShader(GL_VERTEX_SHADER, vs);
        GLuint sFS = CompileShader(GL_FRAGMENT_SHADER, fs);
        if (!sVS || !sFS) throw std::runtime_error("Solid shader compile failed");

        m_solidProg = LinkProgram(sVS, sFS);
        glDeleteShader(sVS);
        glDeleteShader(sFS);
        if (!m_solidProg) throw std::runtime_error("Solid program link failed");

        m_solidLocMVP = glGetUniformLocation(m_solidProg, "uMVP");
        m_solidLocColor = glGetUniformLocation(m_solidProg, "uColor");
        if (m_solidLocMVP < 0 || m_solidLocColor < 0)
            throw std::runtime_error("Solid uniform not found");
    }

    void createCubeSolidMesh()
    {
        auto pos = CreateCubeSolidPositions(0.5f);

        glGenVertexArrays(1, &m_cubeSolidVAO);
        glGenBuffers(1, &m_cubeSolidVBO);

        glBindVertexArray(m_cubeSolidVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeSolidVBO);
        glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(Vec3), pos.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void requestPickIfClicked()
    {
        // ImGuiがマウスを使ってるなら無視
        if (ImGui::GetIO().WantCaptureMouse) return;

        // 左クリック押下の瞬間だけ拾う（簡易：前フレーム状態を保持）
        static int prev = GLFW_RELEASE;
        int now = glfwGetMouseButton(m_window.get(), GLFW_MOUSE_BUTTON_LEFT);
        if (now == GLFW_PRESS && prev == GLFW_RELEASE)
        {
            glfwGetCursorPos(m_window.get(), &m_pickX, &m_pickY);
            m_pickRequested = true;
        }
        prev = now;
    }

    uint32_t doPicking(const Mat4& vp, int fbW, int fbH, double mouseX, double mouseY)
    {
        ensurePickFBO(fbW, fbH);

        glBindFramebuffer(GL_FRAMEBUFFER, m_pickFBO);
        glViewport(0, 0, fbW, fbH);

        // ピッキング中はブレンド不要（むしろ邪魔）
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        GLuint clearID = 0;
        glClearBufferuiv(GL_COLOR, 0, &clearID);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_pickProg);
        glUniformMatrix4fv(m_pickLocMVP, 1, GL_FALSE, vp.m);

        glBindVertexArray(m_cubeSolidVAO);

        // 面ごとに uID を変えて描く（1..6）
        // 1面 = 6頂点（2三角形）
        for (uint32_t face = 0; face < 6; ++face)
        {
            uint32_t id = face + 1;
            glUniform1ui(m_pickLocID, id);
            glDrawArrays(GL_TRIANGLES, (GLint)(face * 6), 6);
        }

        glBindVertexArray(0);
        glUseProgram(0);

        // 読み取り座標：GLは左下原点、GLFWカーソルは左上原点
        int px = (int)mouseX;
        int py = fbH - 1 - (int)mouseY;

        uint32_t out = 0;
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(px, py, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &out);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 元の状態に戻す（あなたは通常ブレンドONなので戻す）
        glEnable(GL_BLEND);

        return out; // 0: none, 1..6: face
    }

    void drawSelectedFaceFill(const Mat4& vp)
    {
        if (m_selectedFace == 0) return;

        // faceID: 1..6
        const uint32_t face = m_selectedFace;
        const GLint first = (GLint)((face - 1) * 6);
        const GLsizei count = 6;

        // Z-fighting対策（面とワイヤが同じ深度にいるのでチラつく）
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);

        // 透明重ね
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(m_solidProg);
        glUniformMatrix4fv(m_solidLocMVP, 1, GL_FALSE, vp.m);

        // ハイライト色（好みで）
        glUniform4f(m_solidLocColor, 1.0f, 0.8f, 0.2f, 0.25f);

        glBindVertexArray(m_cubeSolidVAO);
        glDrawArrays(GL_TRIANGLES, first, count);
        glBindVertexArray(0);

        glUseProgram(0);

        glDisable(GL_POLYGON_OFFSET_FILL);
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
