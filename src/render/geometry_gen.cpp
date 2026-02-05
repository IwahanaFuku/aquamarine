#include "render/geometry_gen.h"

std::vector<Vertex> geometry_gen::generateGrid(int half, float step)
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

std::vector<Vertex> geometry_gen::generateCubeWire(float s)
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

std::vector<glm::vec3> geometry_gen::generateCubeSolidPositions(float s)
{
    // 6 faces * 2 triangles * 3 verts = 36
    std::vector<glm::vec3> v;
    v.reserve(36);

    auto tri = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) { v.push_back(a); v.push_back(b); v.push_back(c); };

    // corners
    glm::vec3 p000{ -s,-s,-s }, p001{ -s,-s, s }, p010{ -s, s,-s }, p011{ -s, s, s };
    glm::vec3 p100{ s,-s,-s }, p101{ s,-s, s }, p110{ s, s,-s }, p111{ s, s, s };

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