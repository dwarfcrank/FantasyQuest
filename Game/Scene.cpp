#include "pch.h"

#include "Scene.h"
#include "Transform.h"
#include "File.h"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace DirectX;

namespace nlohmann
{
    template<>
    struct adl_serializer<DirectX::XMFLOAT3>
    {
        static void to_json(json& j, const DirectX::XMFLOAT3& v)
        {
            j.push_back(v.x);
            j.push_back(v.y);
            j.push_back(v.z);
        }

        static void from_json(const json& j, DirectX::XMFLOAT3& v)
        {
            v.x = j[0].get<float>();
            v.y = j[1].get<float>();
            v.z = j[2].get<float>();
        }
    };

    template<>
    struct adl_serializer<DirectX::XMFLOAT4>
    {
        static void to_json(json& j, const DirectX::XMFLOAT4& v)
        {
            j.push_back(v.x);
            j.push_back(v.y);
            j.push_back(v.z);
            j.push_back(v.w);
        }

        static void from_json(const json& j, DirectX::XMFLOAT4& v)
        {
            v.x = j[0].get<float>();
            v.y = j[1].get<float>();
            v.z = j[2].get<float>();
            v.w = j[3].get<float>();
        }
    };
}

 Object::Object(const RModel& model, const Transform& t) :
    renderable(model.renderable), transform(t), modelName(model.name), bounds(model.bounds)
{
    name = fmt::format("{}:{}", model.name, model.count);
    XMStoreFloat3(&position, t.Position);
    XMStoreFloat3(&rotation, t.Rotation);
    XMStoreFloat3(&scale, t.Scale);
}

void Object::update()
{
    transform.Position = XMLoadFloat3(&position);
    transform.Rotation = XMLoadFloat3(&rotation);
    transform.Scale = XMLoadFloat3(&scale);
}

static void to_json(nlohmann::json& j, const Object& obj)
{
    j["name"] = obj.name;
    j["model"] = obj.modelName;
    j["position"] = obj.position;
    j["rotation"] = obj.rotation;
    j["scale"] = obj.scale;
}

static void from_json(const nlohmann::json& j, Object& obj)
{
    j["name"].get_to(obj.name);
    j["model"].get_to(obj.modelName);
    j["position"].get_to(obj.position);
    j["rotation"].get_to(obj.rotation);
    j["scale"].get_to(obj.scale);

    obj.update();
}

static void to_json(nlohmann::json& j, const PointLight& p)
{
    j["position"] = p.Position;
    j["color"] = p.Color;
}

static void from_json(const nlohmann::json& j, PointLight& p)
{
    j["position"].get_to(p.Position);
    j["color"].get_to(p.Color);
}

static void to_json(nlohmann::json& j, const Scene& s)
{
    j["directionalLight"] = s.directionalLight;
    j["lights"] = s.lights;
    j["objects"] = s.objects;
}

static void from_json(const nlohmann::json& j, Scene& s)
{
    j["directionalLight"].get_to(s.directionalLight);
    j["lights"].get_to(s.lights);
    j["objects"].get_to(s.objects);
}

void Scene::load(const std::filesystem::path& path)
{
    lights.clear();
    objects.clear();

    auto d = loadFile(path);
    nlohmann::json::parse(d).get_to(*this);

    for (const auto& obj : objects) {
        auto entity = reg.create();

        reg.emplace<components::Transform>(entity, components::Transform{
            .position = obj.position,
            .rotation = obj.rotation,
            .scale = obj.scale,
		});

        reg.emplace<components::Renderable>(entity, obj.modelName, nullptr);

        reg.emplace<components::Misc>(entity, obj.name);
    }

    for (auto& light : lights) {
        light.Intensity = 1.0f;
    }
}

void Scene::save(const std::filesystem::path& path) const
{
    nlohmann::json j;
    j = *this;
    std::ofstream(path) << j;
}
