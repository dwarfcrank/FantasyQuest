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

template<typename Archive>
void serialize(Archive& archive, DirectX::XMFLOAT4& v)
{
    archive(v.x, v.y, v.z, v.w);
}

template<typename Archive>
void save(Archive& archive, const DirectX::XMVECTOR& v)
{
    XMFLOAT4 fv;
    XMStoreFloat4(&fv, v);
    archive(fv);
}

template<typename Archive>
void load(Archive& archive, DirectX::XMVECTOR& v)
{
    XMFLOAT4 fv;
    archive(fv);
    v = XMLoadFloat4(&fv);
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
    archive(t.position, t.rotationQuat, t.scale);
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
        .connect<&PhysicsWorld::onCreatePhysicsComponent>(physicsWorld);

    reg
        .on_destroy<components::Physics>()
        .connect<&PhysicsWorld::onDestroyPhysicsComponent>(physicsWorld);

    reg
        .on_construct<components::Collision>()
        .connect<&PhysicsWorld::onCreateCollisionComponent>(physicsWorld);

    reg
        .on_destroy<components::Collision>()
        .connect<&PhysicsWorld::onDestroyCollisionComponent>(physicsWorld);
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
