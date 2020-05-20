#pragma once

#include "Math.h"

#include <vector>
#include <memory>
#include <bullet/btBulletDynamicsCommon.h>
#include <entt/entt.hpp>
#include <string_view>
#include <unordered_map>
#include <string>
#include <absl/container/inlined_vector.h>

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

    struct Collision
    {
        std::unique_ptr<btCollisionObject> collisionObject;
        btCollisionShape* collisionShape = nullptr;
    };
}

// TODO: use Im3d for a lot of these
class PhysicsDebugDraw : public btIDebugDraw
{
public:
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB,
        btScalar distance, int lifeTime, const btVector3& color) override;

    virtual void reportErrorWarning(const char* warningString) override;
    virtual void draw3dText(const btVector3& location, const char* textString) override;
    virtual void setDebugMode(int debugMode) override;
    virtual int getDebugMode() const override;

private:
    int m_debugMode = 0;
};

struct RaycastHit
{
    math::WorldVector position;
    math::WorldVector normal;
    entt::entity entity = entt::null;

    explicit operator bool() const
    {
        return entity != entt::null;
    }
};

using RaycastHitList = absl::InlinedVector<RaycastHit, 1>;

class PhysicsWorld
{
public:
    PhysicsWorld(struct Scene& scene);
    ~PhysicsWorld();

    void onCreatePhysicsComponent(entt::registry&, entt::entity);
    void onDestroyPhysicsComponent(entt::registry&, entt::entity);

    void onCreateCollisionComponent(entt::registry&, entt::entity);
    void onDestroyCollisionComponent(entt::registry&, entt::entity);

    void addBox(float hw, float hh, float hd, float mass, float x, float y, float z);
    void addSphere(float radius, float mass, float x, float y, float z);

    btCollisionShape* createCollisionMesh(const std::string& name, const class Mesh& mesh);
    btCollisionShape* getCollisionMesh(const std::string& name);

    void editorUpdate();
    void update(float dt);

    void setDebugDrawMode(int mode);
    void render();

    RaycastHit raycast(math::WorldVector from, math::WorldVector to);

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

    PhysicsDebugDraw m_debugDraw;
};
