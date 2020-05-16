#pragma once

#include "Math.h"

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    static Camera ortho(float width = 40.0f, float height = 40.0f, float nearZ = 100.0f, float farZ = -10.0f);
    static Camera perspective(float fovY = XM_PI / 2.5f, float aspect = 16.0f / 9.0f, float nearZ = 100.0f, float farZ = 0.01f);

    math::Matrix<math::World, math::View> getViewMatrix() const;
    math::Matrix<math::View, math::World> getInverseViewMatrix() const;

    const math::Matrix<math::View, math::Projection>& getProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

    void move(math::Vector<math::View> direction);
    void move(math::Vector<math::World> direction);

    void setPosition(math::Vector<math::World> position);

    void move(float xOff, float yOff, float zOff);
    void setPosition(float x, float y, float z);

    void rotate(float pitch, float yaw);
    void setRotation(float pitch, float yaw);

    void setDirection(const XMFLOAT3& dir);
    void invertDirection();
    void setDirection(float x, float y, float z);
    void makeOrtho();

    math::Vector<math::World> getPosition() const
    {
        return m_position;
    }

private:
    bool m_useDirection = false;
    float m_pitch = 0.0f;
    float m_yaw = 0.0f;
    
    math::Vector<math::World> m_position{ 0.0f };

    XMVECTOR m_direction = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    math::Matrix<math::View, math::Projection> m_projectionMatrix;
};

