#pragma once

#include "math/math.h"

class OrbitCamera
{
public:
    OrbitCamera();

    // 入力操作
    void orbit(float dx, float dy);
    void pan(float dxPixels, float dyPixels, int fbW, int fbH);
    void zoom(float wheel);

    // 行列取得
    Mat4 viewMatrix() const;

    // デバッグ・UI用
    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    float distance() const { return m_distance; }
    Vec3  target() const { return m_target; }

private:
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_distance = 5.0f;
    Vec3  m_target{ 0.0f, 0.0f, 0.0f };

    Vec3 eyePosition() const;
};
