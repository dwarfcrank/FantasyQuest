#pragma once

#include "Math.h"
#include "Transform.h"

#include <DirectXMath.h>
#include <vector>

struct DebugDrawVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

struct DebugDraw
{
    void clear();

    void drawLine(Vector<World> from, Vector<World> to, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
    void drawAABB(Vector<World> min, Vector<World> max, const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    void drawBounds(Vector<Model> min, Vector<Model> max, const Transform& transform,
        const DirectX::XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    std::vector<DebugDrawVertex> verts;
};
