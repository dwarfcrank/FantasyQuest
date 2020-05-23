#pragma once

#include "../Math.h"
#include <DirectXMath.h>

namespace components
{

struct Transform
{
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	// TODO: need to get rid of this...
	DirectX::XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

	DirectX::XMVECTOR rotationQuat = DirectX::XMQuaternionIdentity();

	DirectX::XMMATRIX getMatrix() const
	{
		auto t = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&position));
		auto r = DirectX::XMMatrixRotationQuaternion(rotationQuat);
		auto s = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&scale));

		return s * r * t;
	}

	math::Matrix<math::Model, math::World> getMatrix2() const
	{
		return math::Matrix<math::Model, math::World>{ getMatrix() };
	}
};

template<typename Archive>
void serialize(Archive& archive, Transform& t)
{
    archive(t.position, t.rotationQuat, t.scale);
}

}
