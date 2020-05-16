#pragma once

#include <vector>
#include <memory>
#include <bullet/btBulletDynamicsCommon.h>
#include <entt/entt.hpp>

namespace components
{
    struct Physics
    {
        std::unique_ptr<btCollisionObject> collisionObject;
        std::unique_ptr<btMotionState> motionState;
        std::unique_ptr<btCollisionShape> collisionShape;
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
    void update(float dt);
    void render();

private:
    struct Scene& m_scene;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlappingPairCache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

    std::vector<std::unique_ptr<btCollisionShape>> m_collisionShapes;
    std::vector<std::unique_ptr<btCollisionObject>> m_collisionObjects;
    std::vector<std::unique_ptr<btMotionState>> m_motionStates;

    float m_time = 0.0f;

    static constexpr auto TICKS_PER_SECOND = 60;
    static constexpr auto TIMESTEP = 1.0f / float(TICKS_PER_SECOND);
};
