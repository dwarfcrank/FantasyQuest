#include "pch.h"

#include "DebugDraw.h"
#include "Renderer.h"
#include "Math.h"

void DebugDraw::clear()
{
    verts.clear();
}

void DebugDraw::drawLine(Vector<World> from, Vector<World> to, const DirectX::XMFLOAT4& color)
{
    DebugDrawVertex v;
    v.Color = color;

    XMStoreFloat3(&v.Position, from.vec);
    verts.push_back(v);

    XMStoreFloat3(&v.Position, to.vec);
    verts.push_back(v);
}

void DebugDraw::drawAABB(Vector<World> min, Vector<World> max, const DirectX::XMFLOAT4& color)
{
    Vector<World> corners[8];

    corners[0] = min;                                                    // 0, 0, 0, 0
    corners[1].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 0, 1, 0));
    corners[2].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 1, 0, 0));
    corners[3].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 1, 1, 0));
    corners[4].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 0, 0, 0));
    corners[5].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 0, 1, 0));
    corners[6].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 1, 0, 0));
    corners[7] = max;                                                    // 1, 1, 1, 0

    drawLine(corners[0], corners[1], color);
    drawLine(corners[0], corners[2], color);
    drawLine(corners[0], corners[4], color);
    drawLine(corners[1], corners[5], color);
    drawLine(corners[2], corners[3], color);
    drawLine(corners[2], corners[6], color);
    drawLine(corners[3], corners[1], color);
    drawLine(corners[4], corners[5], color);
    drawLine(corners[4], corners[6], color);
    drawLine(corners[5], corners[7], color);
    drawLine(corners[6], corners[7], color);
    drawLine(corners[7], corners[3], color);
}

void DebugDraw::drawBounds(Vector<Model> min, Vector<Model> max, const Transform& transform, const DirectX::XMFLOAT4& color)
{
    Vector<Model> corners[8];

    corners[0] = min;                                                    // 0, 0, 0, 0
    corners[1].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 0, 1, 0));
    corners[2].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 1, 0, 0));
    corners[3].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(0, 1, 1, 0));
    corners[4].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 0, 0, 0));
    corners[5].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 0, 1, 0));
    corners[6].vec = XMVectorSelect(min.vec, max.vec, XMVectorSelectControl(1, 1, 0, 0));
    corners[7] = max;                                                    // 1, 1, 1, 0

    auto matrix = transform.getMatrix2();
    Vector<World> transformed[8];
    for (int i = 0; i < 8; i++) {
        transformed[i] = corners[i] * matrix;
    }

    drawLine(transformed[0], transformed[1], color);
    drawLine(transformed[0], transformed[2], color);
    drawLine(transformed[0], transformed[4], color);
    drawLine(transformed[1], transformed[5], color);
    drawLine(transformed[2], transformed[3], color);
    drawLine(transformed[2], transformed[6], color);
    drawLine(transformed[3], transformed[1], color);
    drawLine(transformed[4], transformed[5], color);
    drawLine(transformed[4], transformed[6], color);
    drawLine(transformed[5], transformed[7], color);
    drawLine(transformed[6], transformed[7], color);
    drawLine(transformed[7], transformed[3], color);
}

void DebugDraw::drawSphere(float radius, const Transform& transform, const DirectX::XMFLOAT4& color)
{
    constexpr int segments = 20;

    static std::vector<Vector<Model>> sphereVerts;
    static std::vector<Vector<World>> transformed;
    
    if (sphereVerts.empty()) {
        constexpr float step = XM_2PI / float(segments);

        float t = 0.0f;

        sphereVerts.resize(segments * 3);

        for (int i = 0; i < segments; i++) {
            float x = std::cos(t);
            float y = std::sin(t);

            sphereVerts[i * 3 + 0] = Vector<Model>{ x, y, 0.0f, 1.0f };
            sphereVerts[i * 3 + 1] = Vector<Model>{ x, 0.0f, y, 1.0f };
            sphereVerts[i * 3 + 2] = Vector<Model>{ 0.0f, x, y, 1.0f };

            t += step;
        }
    }

    Transform t2(transform);
    t2.Scale = XMVectorReplicate(radius);

    auto matrix = t2.getMatrix2();

    transformed.clear();

    for (const auto& v : sphereVerts) {
        transformed.push_back(v * matrix);
    }

    for (int i = 0; i < segments; i++) {
        int from = (i % segments) * 3;
        int to = ((i + 1) % segments) * 3;

        drawLine(transformed[from + 0], transformed[to + 0]);
        drawLine(transformed[from + 1], transformed[to + 1]);
        drawLine(transformed[from + 2], transformed[to + 2]);
    }
}
