#pragma once

#include <DirectXMath.h>

namespace components
{

struct PointLight
{
    DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float linearAttenuation = 0.5f;
    float quadraticAttenuation = 0.5f;
    float radius = 10.0f;
};

template<typename Archive>
void serialize(Archive& archive, PointLight& p)
{
    archive(p.color, p.intensity, p.linearAttenuation, p.quadraticAttenuation, p.radius);
}

}
