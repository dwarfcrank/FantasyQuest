#pragma once

#include <vector>
#include <memory>
#include <bullet/btBulletDynamicsCommon.h>
#include <entt/entt.hpp>
#include <string_view>
#include <unordered_map>
#include <string>

namespace components
{
    struct Physics
    {
        Physics(float mass = 5.0f) :
            mass(mass)
        {
        }

        float mass;

        std::unique_ptr<btCollisionObject> collisionObject;
        std::unique_ptr<btMotionState> motionState;
        btCollisionShape* collisionShape = nullptr;
    };
}

class PhysicsWorld
{
public:
    PhysicsWorld(struct Scene& scene);
    ~PhysicsWorld();

    void onCreate(entt::registry&, entt::entity);
    void onDestroy(entt::registry&, entt::entity);
    void addBox(float hw, float hh, float hd, float mass, float x, float y, float z);
    void addSphere(float radius, float mass, float x, float y, float z);

    btCollisionShape* createCollisionMesh(const std::string& name, const class Mesh& mesh);
    btCollisionShape* getCollisionMesh(const std::string& name);

    void update(float dt);
    void render();

private:
    struct Scene& m_scene;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

    std::unordered_map<std::string, btCollisionShape*> m_collisionMeshes;

    std::vector<std::unique_ptr<btCollisionShape>> m_collisionShapes;
    std::vector<std::unique_ptr<btCollisionObject>> m_collisionObjects;
    std::vector<std::unique_ptr<btMotionState>> m_motionStates;
};