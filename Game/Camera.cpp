#include "pch.h"

#include "Camera.h"

Camera Camera::ortho(float width, float height, float nearZ, float farZ)
{
    Camera cam;
    cam.m_projectionMatrix.mat = XMMatrixOrthographicLH(width, height, nearZ, farZ);
    return cam;
}

Camera Camera::perspective(float fovY, float aspect, float nearZ, float farZ)
{
    Camera cam;
    cam.m_projectionMatrix.mat = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    return cam;
}

Matrix<World, View> Camera::getViewMatrix() const
{
    XMVECTOR direction;

    if (m_useDirection) {
        direction = m_direction;
    } else {
        auto rotation = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    }

    return Matrix<World, View>(XMMatrixLookToLH(m_position.vec, direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)));
}

void Camera::move(Vector<View> direction)
{
    auto rotation = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
    Vector<World> worldDir(XMVector3Rotate(direction.vec, rotation));

    move(worldDir);
}

void Camera::move(Vector<World> direction)
{
    m_position += direction;
}

void Camera::move(float xOff, float yOff, float zOff)
{
    move(Vector<View>(xOff, yOff, zOff));
}

void Camera::setPosition(float x, float y, float z)
{
    m_position = Vector<World>(x, y, z);
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
    m_projectionMatrix.mat = XMMatrixOrthographicLH(20.0f, 20.0f, 100.0f, -10.0f);
}
