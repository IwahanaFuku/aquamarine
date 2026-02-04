#include <cstdio>
#include <stdexcept>
#include <memory>

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
        const float kRotateSpeed = 0.005f;
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

        // ---- デバッグUI ----
        ImGui::Begin("Camera Debug");
        ImGui::Text("Yaw    : %.3f", m_camera.m_yaw);
        ImGui::Text("Pitch  : %.3f", m_camera.m_pitch);
        ImGui::Text("Dist   : %.3f", m_camera.m_distance);
        ImGui::Text("Target : %.2f %.2f %.2f",
            m_camera.m_targetX,
            m_camera.m_targetY,
            m_camera.m_targetZ);
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
