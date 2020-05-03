#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    XMMATRIX getViewMatrix() const;
    XMMATRIX getProjectionMatrix() const;

    void move(float xOff, float yOff, float zOff);
    void setPosition(float x, float y, float z);
    void setDirection(const XMFLOAT3& dir);
    void invertDirection();
    void rotate(float pitch, float yaw);
    void setDirection(float x, float y, float z);
    void makeOrtho();

private:
    bool m_useDirection = false;
    float m_pitch = 0.0f;
    float m_yaw = 0.0f;
    
    XMVECTOR m_position = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    XMVECTOR m_direction = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMMATRIX m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PI / 2.5f, 16.0f / 9.0f, 100.0f, 0.01f);
};

