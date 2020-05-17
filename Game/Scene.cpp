#include "pch.h"

#include "Scene.h"
#include "Transform.h"
#include "File.h"

#include <filesystem>
#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

using namespace DirectX;

namespace cereal
{

template<typename Archive>
void serialize(Archive& archive, DirectX::XMFLOAT3& v)
{
    archive(v.x, v.y, v.z);
}

}

namespace components
{

template<typename Archive>
void serialize(Archive& archive, Misc& m)
{
    archive(m.name);
}

template<typename Archive>
void serialize(Archive& archive, Renderable& r)
{
    archive(r.name);
}

template<typename Archive>
void serialize(Archive& archive, Transform& t)
{
    archive(t.position, t.rotation, t.scale);
}

template<typename Archive>
void serialize(Archive& archive, PointLight& p)
{
    archive(p.color, p.intensity, p.linearAttenuation, p.quadraticAttenuation);
}

}

template<typename Archive>
void serialize(Archive& archive, Scene& s)
{
    archive(s.directionalLight);
    archive(s.directionalLightColor);
    archive(s.directionalLightIntensity);
    archive(s.depthBias);
    archive(s.name);
}

Scene::Scene() :
    physicsWorld(*this)
{
    reg
        .on_construct<components::Physics>()
        .connect<&PhysicsWorld::onCreate>(physicsWorld);

    reg
        .on_destroy<components::Physics>()
        .connect<&PhysicsWorld::onDestroy>(physicsWorld);
}

void Scene::load(const std::filesystem::path& path)
{
    std::ifstream input(path);
    cereal::JSONInputArchive archive(input);

    entt::snapshot_loader(reg)
        .entities(archive)
        .component<components::Misc, components::Transform, components::Renderable, components::PointLight>(archive);

    archive(*this);
}

void Scene::save(const std::filesystem::path& path)
{
    std::ofstream o(path);
    cereal::JSONOutputArchive archive(o);

    entt::snapshot(reg)
        .entities(archive)
        .component<components::Misc, components::Transform, components::Renderable, components::PointLight>(archive);

    archive(*this);
}
