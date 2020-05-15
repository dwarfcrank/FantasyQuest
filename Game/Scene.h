#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <filesystem>
#include <entt/entt.hpp>

#include "Transform.h"
#include "Renderer.h"

class Renderable;

namespace components
{
    struct Misc
    {
        Misc() = default;

        Misc(std::string name) :
            name{ std::move(name) } {}

        std::string name;
    };

    struct Renderable
    {
        Renderable(std::string name, ::Renderable* renderable, const Bounds& bounds) :
            name{ std::move(name) }, renderable{ renderable }, bounds{ bounds } {}

        Renderable() = default;

        std::string name;
        ::Renderable* renderable = nullptr;
        Bounds bounds{ Vector<Model>(), Vector<Model>() };
    };

    struct PointLight
    {
        XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
        float intensity = 1.0f;
        float linearAttenuation = 0.5f;
        float quadraticAttenuation = 0.5f;
    };

    struct Transform
    {
        DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 rotation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

		XMMATRIX getMatrix() const
		{
			auto t = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
			auto r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
			auto s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

			return s * r * t;
		}

		Matrix<Model, World> getMatrix2() const
		{
			return Matrix<Model, World>{ getMatrix() };
		}
    };
}

struct Scene
{
    Scene() = default;

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path);

    XMFLOAT3 directionalLight{ 1.0f, 1.0f, -1.0f };
    XMFLOAT3 directionalLightColor{ 1.0f, 1.0f, 1.0f };
    float directionalLightIntensity = 1.0f;
    float depthBias = 0.005f;

    /*
    std::vector<Object> objects;
    std::vector<PointLight> lights;
    */

    entt::registry reg;
};
