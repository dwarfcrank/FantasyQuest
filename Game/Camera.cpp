#include "pch.h"

#include "Camera.h"

using namespace math;

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

Vector<World> Camera::viewToWorld(Vector<View> viewVec) const
{
    auto v = XMVectorSet(
        XMVectorGetX(m_projectionMatrix.mat.r[0]),
        XMVectorGetY(m_projectionMatrix.mat.r[1]),
        1.0f, 1.0f
    );

    viewVec.vec = XMVectorDivide(viewVec.vec, v);
    return viewVec * m_invViewMatrix;
}

void Camera::update()
{
    XMVECTOR direction;

    if (m_useDirection) {
        direction = m_direction;
    } else {
        auto rotation = XMQuaternionRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
        direction = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    }

    m_viewMatrix.mat = XMMatrixLookToLH(m_position.vec, direction, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    m_invViewMatrix.mat = XMMatrixInverse(nullptr, m_viewMatrix.mat);
}
