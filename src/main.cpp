#include <cstdio>
#include <stdexcept>
#include <memory>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// InputState
struct InputState
{
	bool m_leftMouseDown = false;
	bool m_middleMouseDown = false;
	bool m_rightMouseDown = false;

	double m_mouseX = 0.0;
	double m_mouseY = 0.0;

	double m_deltaX = 0.0;
	double m_deltaY = 0.0;

	double m_scrollY = 0.0;

    void beginFrame()
    {
        m_deltaX = 0.0;
        m_deltaY = 0.0;
        m_scrollY = 0.0;
	}
};

// Callbacks
static InputState* GetInput(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
	InputState* input = GetInput(window);
	if (!input) return;

	const bool down = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_LEFT)   input->m_leftMouseDown = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) input->m_middleMouseDown = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT)  input->m_rightMouseDown = (action == GLFW_PRESS);
}

static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    InputState* input = GetInput(window);
    if (!input) return;

    input->m_deltaX += xpos - input->m_mouseX;
    input->m_deltaY += ypos - input->m_mouseY;
    input->m_mouseX = xpos;
    input->m_mouseY = ypos;
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)xoffset;
    InputState* input = GetInput(window);
    if (!input) return;

    input->m_scrollY += yoffset;
}

// OrbitCamera
struct Vec3
{
    float x, y, z;

    Vec3 operator+(const Vec3& r) const { return { x + r.x, y + r.y, z + r.z }; }
    Vec3 operator-(const Vec3& r) const { return { x - r.x, y - r.y, z - r.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};

static float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
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
    {
        for (int r0 = 0; r0 < 4; ++r0)
        {
            r.m[c * 4 + r0] =
                a.m[0 * 4 + r0] * b.m[c * 4 + 0] +
                a.m[1 * 4 + r0] * b.m[c * 4 + 1] +
                a.m[2 * 4 + r0] * b.m[c * 4 + 2] +
                a.m[3 * 4 + r0] * b.m[c * 4 + 3];
        }
    }
    return r;
}

// OpenGLでよくある lookAt（右手系）
static Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up)
{
    // カメラの前方向（centerへ向かう）
    Vec3 f = Normalize(center - eye);
    // 右方向
    Vec3 s = Normalize(Cross(f, up));
    // 上方向（正規直交化）
    Vec3 u = Cross(s, f);

    Mat4 r = Mat4::Identity();

    // 列優先で配置（OpenGLの一般形）
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;

    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;

    // 右手系なので -f を入れる
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;

    r.m[12] = -Dot(s, eye);
    r.m[13] = -Dot(u, eye);
    r.m[14] = Dot(f, eye); // -Dot(-f, eye) と同じ

    return r;
}

static Mat4 PerspectiveRH(float fovY, float aspect, float zNear, float zFar)
{
    // 視野角の強さ
    const float f = 1.0f / std::tan(fovY * 0.5f);

    Mat4 r{};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    // r.m[15] = 0.0f; // 省略（0初期化済み）

    return r;
}

struct OrbitCamera
{
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_distance = 5.0f;

    float m_targetX = 0.0f;
    float m_targetY = 0.0f;
    float m_targetZ = 0.0f;

    void orbit(float deltaX, float deltaY)
    {
        const float kRotateSpeed = 0.001f;
        m_yaw += deltaX * kRotateSpeed;
        m_pitch += deltaY * kRotateSpeed;

        // Clamp pitch
        const float kLimit = 1.553343f; // 約 89度 = pi/2 - 0.01745
        if (m_pitch > kLimit) m_pitch = kLimit;
        if (m_pitch < -kLimit) m_pitch = -kLimit;
    }

    void zoom(float wheel)
    {
        const float kZoom = 0.9f;
        if (wheel > 0.0f) m_distance *= kZoom;
        if (wheel < 0.0f) m_distance /= kZoom;

        // 距離の下限/上限
        if (m_distance < 0.1f)  m_distance = 0.1f;
        if (m_distance > 200.f) m_distance = 200.f;
    }

    void pan(float dx, float dy)
    {
        // 本当はカメラの右/上ベクトルで動かすのが正しいが、
        // まずは「値が動く」最小としてXY平面でパンさせる
        const float kPan = 0.01f * m_distance;
        m_targetX += -dx * kPan;
        m_targetY += dy * kPan;
    }

    Vec3 target() const
    {
        return { m_targetX, m_targetY, m_targetZ };
	}

    Vec3 eye() const
    {
        // yaw/pitch からターゲット→カメラ方向
        const float cy = std::cos(m_yaw);
        const float sy = std::sin(m_yaw);
        const float cp = std::cos(m_pitch);
        const float sp = std::sin(m_pitch);

        Vec3 dir = { cp * sy, sp, cp * cy }; // 単位ベクトルになる
        return target() + dir * m_distance;
	}

    Mat4 viewMatrix() const
    {
        const Vec3 up = { 0.0f, 1.0f, 0.0f };
        return LookAtRH(eye(), target(), up);
    }
};

// GlfwSystem
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

    ~GlfwSystem()
    {
        glfwTerminate();
    }

    GlfwSystem(const GlfwSystem&) = delete;
    GlfwSystem& operator=(const GlfwSystem&) = delete;
};

// UniqueGlfwWindow
struct GlfwWindowDeleter
{
    void operator()(GLFWwindow* p) const noexcept
    {
        if (p) glfwDestroyWindow(p);
    }
};

using UniqueGlfwWindow = std::unique_ptr<GLFWwindow, GlfwWindowDeleter>;

// ImGuiContextGuard
class ImGuiContextGuard
{
public:
    ImGuiContextGuard(GLFWwindow* window, const char* glslVersion)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glslVersion);
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

// GlfwWindowManager
class GlfwWindowManager
{
private:
    GlfwSystem m_glfw;
    UniqueGlfwWindow m_window;
    std::unique_ptr<ImGuiContextGuard> m_imgui;
	InputState m_input;
	OrbitCamera m_camera;

public:
    GlfwWindowManager()
    {
        setWindowHints();

        m_window = createWindow(1280, 720, "MyModeler");
        glfwMakeContextCurrent(m_window.get());
        glfwSwapInterval(1);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to initialize glad");

        m_imgui = std::make_unique<ImGuiContextGuard>(
            m_window.get(), "#version 330");

		SetCallbacks();
    }

    void SetCallbacks()
    {
        glfwSetWindowUserPointer(m_window.get(), &m_input);
        glfwSetMouseButtonCallback(m_window.get(), MouseButtonCallback);
        glfwSetCursorPosCallback(m_window.get(), CursorPosCallback);
        glfwSetScrollCallback(m_window.get(), ScrollCallback);
	}

    void mainLoop()
    {
        while (!glfwWindowShouldClose(m_window.get()))
        {
            glfwPollEvents();

            startNewFrame();

            content();

            render();
        }
    }

private:
    static void setWindowHints()
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    UniqueGlfwWindow createWindow(int w, int h, const char* title)
    {
        GLFWwindow* raw = glfwCreateWindow(w, h, title, nullptr, nullptr);
        if (!raw)
            throw std::runtime_error("Failed to create GLFW window");
        return UniqueGlfwWindow(raw);
    }

    static void startNewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	}

    void content()
    {
		// ここに描画コードを追加
        ImGuiIO& io = ImGui::GetIO();
        const bool imguiWantsMouse = io.WantCaptureMouse;

        // ---- カメラ操作（ImGuiがマウスを使っていないときのみ） ----
        if (!imguiWantsMouse)
        {
            if (m_input.m_leftMouseDown)
                m_camera.orbit((float)m_input.m_deltaX, (float)m_input.m_deltaY);

            if (m_input.m_middleMouseDown)
                m_camera.pan((float)m_input.m_deltaX, (float)m_input.m_deltaY);

            m_camera.zoom((float)m_input.m_scrollY);
        }

        const Vec3 e = m_camera.eye();
        const Mat4 v = m_camera.viewMatrix();

        // フレームバッファサイズ
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);
        float aspect = (h > 0) ? (float)w / (float)h : 1.0f;

        // Camera matrices
        Mat4 view = m_camera.viewMatrix();
        Mat4 proj = PerspectiveRH(
            60.0f * 3.1415926f / 180.0f, // fovY = 60°
            aspect,
            0.1f,   // near
            1000.0f // far
        );

        Mat4 vp = Mul(proj, view);

        // ---- デバッグUI ----
        ImGui::SetNextWindowSize(ImVec2(400, 220), ImGuiCond_Once);
        ImGui::Begin("Camera Debug");

        ImGui::Text("Yaw    : %.3f", m_camera.m_yaw);
        ImGui::Text("Pitch  : %.3f", m_camera.m_pitch);
        ImGui::Text("Dist   : %.3f", m_camera.m_distance);
        ImGui::Text("Target : %.2f %.2f %.2f",
            m_camera.m_targetX,
            m_camera.m_targetY,
            m_camera.m_targetZ);

        ImGui::Separator();

        ImGui::Text("Eye   : %.2f %.2f %.2f", e.x, e.y, e.z);
        ImGui::Text("View m[12..14] (translation): %.3f %.3f %.3f", v.m[12], v.m[13], v.m[14]);

		ImGui::Separator();

        ImGui::Text("View T : %.2f %.2f %.2f",
            view.m[12], view.m[13], view.m[14]);

        ImGui::Text("Proj m[0] (f/aspect): %.2f", proj.m[0]);
        ImGui::Text("Proj m[5] (f)       : %.2f", proj.m[5]);
        ImGui::Text("Proj m[10] (z)      : %.2f", proj.m[10]);

        ImGui::End();
    }

    void render()
    {
        int w, h;
        glfwGetFramebufferSize(m_window.get(), &w, &h);

        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window.get());
    }
};

int main()
{
    GlfwWindowManager manager;
    manager.mainLoop();
    return 0;
}
