#include "pch.h"

#include "Camera.h"

using namespace math;

Camera::Camera(XMFLOAT2 viewportSize, float nearZ, float farZ) :
    m_viewportSize(viewportSize), m_nearZ(nearZ), m_farZ(farZ)
{
}

Camera Camera::ortho(XMFLOAT2 viewportSize, float width, float height, float nearZ, float farZ)
{
    Camera cam(viewportSize, nearZ, farZ);
    // NOTE: near/far are intentionally swapped here because of the reverse-Z depth buffer
    cam.m_projectionMatrix.mat = XMMatrixOrthographicLH(width, height, farZ, nearZ);
    return cam;
}

Camera Camera::perspective(XMFLOAT2 viewportSize, float fov, float nearZ, float farZ)
{
    Camera cam(viewportSize, nearZ, farZ);

    float fovY = 2.0f * std::atan(std::tan(fov / 2.0f) * (viewportSize.y / viewportSize.x));

    // NOTE: near/far are intentionally swapped here because of the reverse-Z depth buffer
    cam.m_projectionMatrix.mat = XMMatrixPerspectiveFovLH(fovY, viewportSize.x / viewportSize.y, farZ, nearZ);
    return cam;
}

void Camera::move(math::ViewVector v)
{
    m_position.vec += XMVector3Rotate(v.vec, XMQuaternionRotationRollPitchYawFromVector(m_angles));
}

void Camera::move(math::WorldVector offset)
{
    m_position += offset;
}

void Camera::setPosition(math::WorldVector position)
{
    m_position = position;
}

void Camera::rotate(float pitch, float yaw, float roll)
{
    m_angles = XMVectorAddAngles(m_angles, XMVectorSet(pitch, yaw, roll, 0.0f));
}

void Camera::setRotation(float pitch, float yaw, float roll)
{
    m_angles = XMVectorSet(pitch, yaw, roll, 0.0f);
}

void Camera::update()
{
    const auto up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    const auto forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    auto rotation = XMQuaternionRotationRollPitchYawFromVector(m_angles);
    const auto dir = XMVector3Rotate(forward, rotation);

    m_viewMatrix.mat = XMMatrixLookToLH(m_position.vec, dir, up);
    m_invViewMatrix.mat = XMMatrixInverse(nullptr, m_viewMatrix.mat);
}

math::WorldVector Camera::viewToWorld(math::ViewVector viewVec) const
{
    auto v = XMVectorSet(
        XMVectorGetX(m_projectionMatrix.mat.r[0]),
        XMVectorGetY(m_projectionMatrix.mat.r[1]),
        1.0f, 1.0f
    );

    viewVec.vec = XMVectorDivide(viewVec.vec, v);
    return viewVec * m_invViewMatrix;
}

WorldVector Camera::pixelToWorldDirection(int x, int y) const
{
    XMFLOAT2 screenPos(
        (float(x) / m_viewportSize.x) * 2.0f - 1.0f,
        -(float(y) / m_viewportSize.y) * 2.0f + 1.0f
    );

    auto dir = viewToWorld({ screenPos.x, screenPos.y, 1.0f, 1.0f }) - m_position;
    return dir.normalized();
}

