#include "camera/orbit_camera.h"

#include "cmath"

OrbitCamera::OrbitCamera() = default;

void OrbitCamera::orbit(float dx, float dy)
{
    constexpr float k = 0.001f;
    m_yaw -= dx * k;
    m_pitch += dy * k;

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
    Mat4 view = viewMatrix();

    Vec3 right{ view.m[0], view.m[4], view.m[8] };
    Vec3 up{ view.m[1], view.m[5], view.m[9] };

    const float fovY = 60.0f * 3.1415926f / 180.0f;
    const float worldPerPixel =
        (2.0f * m_distance * std::tan(fovY * 0.5f)) / float(fbH);

    m_target = m_target
        + right * (-dxPixels * worldPerPixel)
        + up * (dyPixels * worldPerPixel);
}

Mat4 OrbitCamera::viewMatrix() const
{
    return LookAtRH(eyePosition(), m_target, { 0,1,0 });
}

Vec3 OrbitCamera::eyePosition() const
{
    const float cy = std::cos(m_yaw);
    const float sy = std::sin(m_yaw);
    const float cp = std::cos(m_pitch);
    const float sp = std::sin(m_pitch);

    Vec3 dir{ cp * sy, sp, cp * cy };
    return m_target + dir * m_distance;
}
