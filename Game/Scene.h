#pragma once

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <filesystem>

#include "Transform.h"
#include "Renderer.h"

class Renderable;

struct RModel
{
    RModel(std::string name, Renderable* renderable, const Bounds& bounds) :
        name{ std::move(name) }, renderable(renderable), bounds(bounds)
    {
    }

    Bounds bounds;
    std::string name;
    Renderable* renderable;
    int count = 0;
};

struct Object
{
    Object() = default;

    Object(const RModel& model, const Transform& t);
    void update();

    Renderable* renderable = nullptr;

    Bounds bounds;
    std::string name;
    std::string modelName;
    Transform transform;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 rotation;
    DirectX::XMFLOAT3 scale;
};

struct Scene
{
    Scene() = default;

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;

    XMFLOAT3 directionalLight{ 1.0f, 1.0f, -1.0f };
    XMFLOAT3 directionalLightColor{ 1.0f, 1.0f, 1.0f };
    float depthBias = 0.005f;

    std::vector<Object> objects;
    std::vector<PointLight> lights;
};
