#pragma once

#include <DirectXMath.h>

namespace cereal
{

template<typename Archive>
void serialize(Archive& archive, DirectX::XMFLOAT2& v)
{
    archive(v.x, v.y);
}

template<typename Archive>
void serialize(Archive& archive, DirectX::XMFLOAT3& v)
{
    archive(v.x, v.y, v.z);
}

template<typename Archive>
void serialize(Archive& archive, DirectX::XMFLOAT4& v)
{
    archive(v.x, v.y, v.z, v.w);
}

template<typename Archive>
void save(Archive& archive, const DirectX::XMVECTOR& v)
{
    DirectX::XMFLOAT4 fv;
    DirectX::XMStoreFloat4(&fv, v);
    archive(fv);
}

template<typename Archive>
void load(Archive& archive, DirectX::XMVECTOR& v)
{
    DirectX::XMFLOAT4 fv;
    archive(fv);
    v = DirectX::XMLoadFloat4(&fv);
}

}

