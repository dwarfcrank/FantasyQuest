#include "pch.h"

#include "Transform.h"
#include "PhysicsWorld.h"
#include "im3d.h"
#include "Scene.h"
#include "Mesh.h"

#include <bullet/btBulletDynamicsCommon.h>

// TODO: use Im3d for a lot of these
class PhysicsDebugDraw : public btIDebugDraw
{
public:
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override
    {
        u32 c2 = 0x00'00'00'ff;
        c2 |= (u32(color.x() * 255.0f) << 24);
        c2 |= (u32(color.y() * 255.0f) << 16);
        c2 |= (u32(color.z() * 255.0f) << 8);

        Im3d::DrawLine(
            Im3d::Vec3(from.x(), from.y(), from.z()),
            Im3d::Vec3(to.x(), to.y(), to.z()),
            2.0f, c2
        );
    }

    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB,
        btScalar distance, int lifeTime, const btVector3& color) override
    {
    }

	virtual void reportErrorWarning(const char* warningString) override
    {
    }

    virtual void draw3dText(const btVector3& location, const char* textString) override
    {
    }

    virtual void setDebugMode(int debugMode) override
    {
        m_debugMode = debugMode;
    }

    virtual int getDebugMode() const override
    {
        return m_debugMode;
    }

private:
    int m_debugMode = 0;
};

static PhysicsDebugDraw g_debugDraw;

PhysicsWorld::PhysicsWorld(Scene& scene) :
    m_scene(scene)
{
    m_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());
    m_overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(), m_overlappingPairCache.get(), m_solver.get(), m_collisionConfiguration.get());

    m_dynamicsWorld->setGravity(btVector3(0.0f, -10.0f, 0.0f));
    m_dynamicsWorld->setDebugDrawer(&g_debugDraw);

    auto shape = m_collisionShapes.emplace_back(std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f))).get();
    m_collisionMeshes["basic_box"] = shape;
}

PhysicsWorld::~PhysicsWorld()
{
    for (auto&& obj : m_collisionObjects) {
        m_dynamicsWorld->removeCollisionObject(obj.get());
    }
}

void PhysicsWorld::onCreate(entt::registry& reg, entt::entity entity)
{
    // TODO: most of this should be handled in the Physics component constructor
    // and this should just add the component to the simulation
    auto [tc, pc] = reg.get<components::Transform, components::Physics>(entity);

    pc.collisionShape = getCollisionMesh("basic_box");

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(tc.position.x, tc.position.y, tc.position.z));

    btQuaternion rot;
    rot.set128(tc.rotationQuat);
    transform.setRotation(rot);

    btVector3 localInertia(0.0f, 0.0f, 0.0f);
    if (pc.mass != 0.0f) {
        pc.collisionShape->calculateLocalInertia(pc.mass, localInertia);
    }

    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo info(pc.mass, motionState.get(), pc.collisionShape, localInertia);
    auto body = std::make_unique<btRigidBody>(info);

    body->setUserPointer((void*)(uintptr_t)entity);

    m_dynamicsWorld->addRigidBody(body.get());
    pc.collisionObject = std::move(body);
    pc.motionState = std::move(motionState);
}

void PhysicsWorld::onDestroy(entt::registry& reg, entt::entity entity)
{
    auto& pc = reg.get<components::Physics>(entity);
    m_dynamicsWorld->removeCollisionObject(pc.collisionObject.get());
}

void PhysicsWorld::addBox(float hw, float hh, float hd, float mass, float x, float y, float z)
{
    auto shape = m_collisionShapes.emplace_back(new btBoxShape(btVector3(hw, hh, hd))).get();

    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(x, y, z));

    auto motionState = std::make_unique<btDefaultMotionState>(t);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape);

    auto body = std::make_unique<btRigidBody>(info);
    body->setUserPointer(nullptr);

    m_dynamicsWorld->addRigidBody(body.get());
    m_collisionObjects.emplace_back(std::move(body));
    m_motionStates.emplace_back(std::move(motionState));
}

void PhysicsWorld::addSphere(float radius, float mass, float x, float y, float z)
{
    auto shape = m_collisionShapes.emplace_back(new btSphereShape(radius)).get();

    btTransform t;
    t.setIdentity();
    t.setOrigin(btVector3(x, y, z));

    btVector3 localInertia(0.0f, 0.0f, 0.0f);
    shape->calculateLocalInertia(mass, localInertia);

    auto motionState = std::make_unique<btDefaultMotionState>(t);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape, localInertia);
    auto body = std::make_unique<btRigidBody>(info);
    body->setUserPointer(nullptr);

    m_dynamicsWorld->addRigidBody(body.get());
    m_collisionObjects.emplace_back(std::move(body));
    m_motionStates.emplace_back(std::move(motionState));
}

btCollisionShape* PhysicsWorld::createCollisionMesh(const std::string& name, const Mesh& mesh)
{
    assert(!m_collisionMeshes.contains(name));

    const auto& verts = mesh.getVertices();

    auto shape = std::make_unique<btConvexHullShape>(&verts[0].Position.x, int(verts.size()),
        int(sizeof(Vertex)));
    shape->optimizeConvexHull();

    m_collisionMeshes[name] = shape.get();

    return m_collisionShapes.emplace_back(std::move(shape)).get();
}

btCollisionShape* PhysicsWorld::getCollisionMesh(const std::string& name)
{
    if (auto it = m_collisionMeshes.find(name); it != m_collisionMeshes.end()) {
        return it->second;
    }

    return nullptr;
}

void PhysicsWorld::update(float dt)
{
    m_dynamicsWorld->stepSimulation(dt, 10);

    m_scene.reg.view<components::Transform, components::Physics>()
        .each([&](entt::entity entity, const components::Transform&, const components::Physics& pc) {
            btTransform ft;

            if (pc.motionState) {
                pc.motionState->getWorldTransform(ft);
            } else {
                ft = pc.collisionObject->getWorldTransform();
            }

            const auto& origin = ft.getOrigin();

            m_scene.reg.patch<components::Transform>(entity, [&](components::Transform& tc) {
                tc.position = XMFLOAT3(origin.x(), origin.y(), origin.z());
                tc.rotationQuat = ft.getRotation().get128();
            });
        });
}

void PhysicsWorld::render()
{
    Im3d::PushMatrix(Im3d::Mat4(1.0f));
    m_dynamicsWorld->debugDrawWorld();
    Im3d::PopMatrix();
}
