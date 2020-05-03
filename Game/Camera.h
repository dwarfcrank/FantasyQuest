#pragma once

#include "Math.h"

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    static Camera ortho(float width = 40.0f, float height = 40.0f, float nearZ = 100.0f, float farZ = -10.0f);
    static Camera perspective(float fovY = XM_PI / 2.5f, float aspect = 16.0f / 9.0f, float nearZ = 100.0f, float farZ = 0.01f);

    Matrix<World, View> getViewMatrix() const;

    const Matrix<View, Projection>& getProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

    void move(Vector<View> direction);
    void move(Vector<World> direction);

    void setPosition(Vector<World> position);

    void move(float xOff, float yOff, float zOff);
    void setPosition(float x, float y, float z);

    void rotate(float pitch, float yaw);
    void setRotation(float pitch, float yaw);

    void setDirection(const XMFLOAT3& dir);
    void invertDirection();
    void setDirection(float x, float y, float z);
    void makeOrtho();

private:
    bool m_useDirection = false;
    float m_pitch = 0.0f;
    float m_yaw = 0.0f;
    
    //XMVECTOR m_position = XMVectorZero();
    Vector<World> m_position{ 0.0f };

    XMVECTOR m_direction = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    Matrix<View, Projection> m_projectionMatrix;
};

