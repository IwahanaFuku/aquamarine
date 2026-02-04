#include "platform.h"
#include "platform/input.h"

Platform::Platform()
{
    setWindowHints();
    m_window = createWindow(1280, 720, "MyModeler");

    glfwMakeContextCurrent(m_window.get());
    glfwSwapInterval(1);

    Input::InstallInputCallbacks(m_window.get(), &m_input);
}

Platform::~Platform() = default;

void Platform::beginFrame()
{
    m_input.beginFrame();
    glfwPollEvents();
}

bool Platform::shouldClose() const
{
    return glfwWindowShouldClose(m_window.get()) != 0;
}
