#include "camera/orbit_camera.h"

#include "cmath"
#include "glm/gtc/matrix_transform.hpp"

OrbitCamera::OrbitCamera() = default;

void OrbitCamera::orbit(float dx, float dy)
{
    constexpr float speed = 0.005f;
    m_yaw -= dx * speed;
    m_pitch += dy * speed;

    constexpr float limit = 1.553343f; // ~89deg
    if (m_pitch > limit) m_pitch = limit;
    if (m_pitch < -limit) m_pitch = -limit;
}

void OrbitCamera::zoom(float wheel)
{
    constexpr float k = 0.9f;
    if (wheel > 0) m_distance *= k;
    if (wheel < 0) m_distance /= k;

    if (m_distance < 0.1f)  m_distance = 0.1f;
    if (m_distance > 200.f) m_distance = 200.f;
}

void OrbitCamera::pan(float dxPixels, float dyPixels, int fbW, int fbH)
{
    if (fbH <= 0) return;

    // camera basis in WORLD
    const glm::vec3 eye = eyePosition();
    const glm::vec3 fwd = glm::normalize(m_target - eye);          // eye->target
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(fwd, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, fwd));

    // pixels -> world scale
    const float fovY = glm::radians(60.0f);
    constexpr float speed = 4.0f;
    const float worldPerPixel = (speed * m_distance * std::tan(fovY * 0.5f)) / float(fbH);

    // “掴んで動かす”感覚（一般的）
    m_target += right * (-dxPixels * worldPerPixel);
    m_target += up * (dyPixels * worldPerPixel);
}

glm::mat4 OrbitCamera::viewMatrix() const
{
    const glm::vec3 eye = eyePosition();
    const glm::vec3 center = m_target;
    const glm::vec3 up = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f));

    return glm::lookAt(eye, center, up);
}

glm::vec3 OrbitCamera::eyePosition() const
{
    float cy = std::cos(m_yaw);
    float sy = std::sin(m_yaw);
    float cp = std::cos(m_pitch);
    float sp = std::sin(m_pitch);

    glm::vec3 dir{
        cp * sy,
        sp,
        cp * cy
    };

    return m_target + dir * m_distance;
}
