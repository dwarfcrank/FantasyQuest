#pragma once

#include "Math.h"

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
public:
    Camera() = default;

    static Camera ortho(XMFLOAT2 viewportSize, float width = 80.0f, float height = 80.0f,
        float nearZ = -50.0f, float farZ = 200.0f);

    static Camera perspective(XMFLOAT2 viewportSize, float fov = XM_PI / 2.0f,
        float nearZ = 0.01f, float farZ = 1000.0f);

    XMFLOAT2 getViewportSize() const { return m_viewportSize; }
    math::WorldVector getPosition() const { return m_position; }
    XMVECTOR getAngles() const { return m_angles; }

    void move(math::ViewVector);
    void move(math::WorldVector);
    void setPosition(math::WorldVector);

    void rotate(float pitch, float yaw, float roll = 0.0f);
    void setRotation(float pitch, float yaw, float roll = 0.0f);

    // Recomputes all the matrices
    void update();

    math::WorldVector viewToWorld(math::ViewVector) const;

    const math::Matrix<math::View, math::Projection>& getProjectionMatrix() const { return m_projectionMatrix; }
    const math::Matrix<math::World, math::View>& getViewMatrix() const { return m_viewMatrix; };
    const math::Matrix<math::View, math::World>& getInverseViewMatrix() const { return m_invViewMatrix; }

    math::WorldVector pixelToWorldDirection(int x, int y) const;

    float getFOV() const { return m_fov; }

private:
    Camera(XMFLOAT2 viewportSize, float nearZ, float farZ);

    XMFLOAT2 m_viewportSize;

    float m_nearZ;
    float m_farZ;
    float m_fov;

    math::WorldVector m_position{ 0.0f, 0.0f, 0.0f, 1.0f };

    XMVECTOR m_angles = XMVectorZero();

    math::Matrix<math::View, math::Projection> m_projectionMatrix{ XMMatrixIdentity() };
    math::Matrix<math::World, math::View> m_viewMatrix{ XMMatrixIdentity() };
    math::Matrix<math::View, math::World> m_invViewMatrix{ XMMatrixIdentity() };
};

