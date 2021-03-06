#include "pch.h"

#include "Scene.h"
#include "Transform.h"
#include "File.h"

#include "Components/BasicProperties.h"
#include "Components/Transform.h"
#include "Components/Renderable.h"
#include "Components/PointLight.h"
#include "Serialization.h"

#include <filesystem>
#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

using namespace DirectX;

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
