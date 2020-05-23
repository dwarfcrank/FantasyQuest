#pragma once

#include <DirectXMath.h>
#include <string>
#include <filesystem>
#include <entt/entt.hpp>

#include "PhysicsWorld.h"

struct Scene
{
    Scene();

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path);

    DirectX::XMFLOAT3 directionalLight{ 1.0f, 1.0f, -1.0f };
    DirectX::XMFLOAT3 directionalLightColor{ 1.0f, 1.0f, 1.0f };
    float directionalLightIntensity = 1.0f;
    float depthBias = 0.005f;

    std::string name{ "scene" };
    entt::registry reg;
    PhysicsWorld physicsWorld;
};
