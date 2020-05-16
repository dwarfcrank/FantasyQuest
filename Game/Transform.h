#pragma once

#include "Math.h"
#include <DirectXMath.h>

using namespace DirectX;

struct Transform
{
    XMVECTOR Position = XMVectorZero();
    XMVECTOR Rotation = XMVectorZero();
    XMVECTOR Scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);

    XMMATRIX getMatrix() const
    {
        auto t = XMMatrixTranslationFromVector(Position);
        auto r = XMMatrixRotationRollPitchYawFromVector(Rotation);
        auto s = XMMatrixScalingFromVector(Scale);

        return s * r * t;
    }

    math::Matrix<math::Model, math::World> getMatrix2() const
    {
        return math::Matrix<math::Model, math::World>{ getMatrix() };
    }

    void move(float xOff, float yOff, float zOff)
    {
        //Position += XMVectorSet(xOff, yOff, zOff, 0.0f);
        move(XMVectorSet(xOff, yOff, zOff, 0.0f));
    }

    void move(XMVECTOR direction)
    {
        Position += direction;
    }

    void rotate(float xOff, float yOff, float zOff)
    {
        Rotation += XMVectorSet(xOff, yOff, zOff, 0.0f);
    }
};
