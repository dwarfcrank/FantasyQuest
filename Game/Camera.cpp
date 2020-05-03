#include "pch.h"

#include "Camera.h"

Camera Camera::ortho(float width, float height, float nearZ, float farZ)
{
    Camera cam;
    cam.m_projectionMatrix = XMMatrixOrthographicLH(width, height, nearZ, farZ);
    return cam;
}

Camera Camera::perspective(float fovY, float aspect, float nearZ, float farZ)
{
    Camera cam;
    cam.m_projectionMatrix = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    return cam;
}

XMMATRIX Camera::getViewMatrix() const
{
    XMVECTOR direction;

    if (m_useDirection) {
        direction = m_direction;
    } else {
        auto rotation = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    }

    return XMMatrixLookToLH(m_position, direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
}

XMMATRIX Camera::getProjectionMatrix() const
{
    return m_projectionMatrix;
}

void Camera::move(float xOff, float yOff, float zOff)
{
    auto rotation = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    auto direction = XMVector3Rotate(XMVectorSet(xOff, yOff, zOff, 0.0f), rotation);
    m_position += direction;
}

void Camera::setPosition(float x, float y, float z)
{
    m_position = XMVectorSet(x, y, z, 0.0f);
}

void Camera::setDirection(const XMFLOAT3& dir)
{
    m_useDirection = true;
    m_direction = XMLoadFloat3(&dir);
    m_direction = XMVector3Normalize(m_direction);
}

void Camera::invertDirection()
{
    m_direction = -m_direction;
}

void Camera::rotate(float pitch, float yaw)
{
    m_pitch += pitch;
    m_yaw += yaw;
}

void Camera::setRotation(float pitch, float yaw)
{
    m_pitch = pitch;
    m_yaw = yaw;
}

void Camera::setDirection(float x, float y, float z)
{
    m_direction = XMVectorSet(x, y, z, 0.0f);
}

void Camera::makeOrtho()
{
    m_projectionMatrix = XMMatrixOrthographicLH(20.0f, 20.0f, 100.0f, -10.0f);
}
