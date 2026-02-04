#pragma once

#include "cmath"

struct Vec3
{
    float x, y, z;
    Vec3 operator+(const Vec3& r) const { return { x + r.x, y + r.y, z + r.z }; }
    Vec3 operator-(const Vec3& r) const { return { x - r.x, y - r.y, z - r.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
};

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline Vec3 Normalize(const Vec3& v)
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

inline Mat4 Mul(const Mat4& a, const Mat4& b)
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

inline Mat4 LookAtRH(const Vec3& eye, const Vec3& center, const Vec3& up)
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

inline Mat4 PerspectiveRH(float fovY, float aspect, float zNear, float zFar)
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