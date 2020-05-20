#include "pch.h"

#include "Transform.h"
#include "PhysicsWorld.h"
#include "im3d.h"
#include "Scene.h"
#include "Mesh.h"
#include "Math.h"

#include <absl/container/inlined_vector.h>
#include <bullet/btBulletDynamicsCommon.h>

using namespace math;

static void setEntity(btCollisionObject* obj, entt::entity e)
{
    obj->setUserIndex(int(e));
}

static entt::entity getEntity(const btCollisionObject* obj)
{
    if (auto idx = obj->getUserIndex(); idx != -1) {
        return entt::entity{ idx };
    }

    return entt::null;
}

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
    m_dynamicsWorld->setDebugDrawer(&m_debugDraw);

    auto shape = m_collisionShapes.emplace_back(std::make_unique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f))).get();
    m_collisionMeshes["basic_box"] = shape;
}

PhysicsWorld::~PhysicsWorld()
{
    for (auto&& obj : m_collisionObjects) {
        m_dynamicsWorld->removeCollisionObject(obj.get());
    }
}

void PhysicsWorld::onCreatePhysicsComponent(entt::registry& reg, entt::entity entity)
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

    setEntity(body.get(), reg.entity(entity));

    m_dynamicsWorld->addRigidBody(body.get());
    pc.collisionObject = std::move(body);
    pc.motionState = std::move(motionState);
}

void PhysicsWorld::onDestroyPhysicsComponent(entt::registry& reg, entt::entity entity)
{
    auto& pc = reg.get<components::Physics>(entity);
    m_dynamicsWorld->removeCollisionObject(pc.collisionObject.get());
}

void PhysicsWorld::onCreateCollisionComponent(entt::registry& reg, entt::entity entity)
{
    // TODO: most of this should be handled in the Physics component constructor
    // and this should just add the component to the simulation
    auto [tc, cc] = reg.get<components::Transform, components::Collision>(entity);

    cc.collisionShape = getCollisionMesh("basic_box");

    btQuaternion rot;
    rot.set128(tc.rotationQuat);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(tc.position.x, tc.position.y, tc.position.z));
    transform.setRotation(rot);

    auto obj = std::make_unique<btCollisionObject>();
    //obj->setWorldTransform(transform);
    obj->setWorldTransform(btTransform(rot, btVector3(tc.position.x, tc.position.y, tc.position.z)));
    obj->setCollisionShape(cc.collisionShape);

    setEntity(obj.get(), reg.entity(entity));
    m_dynamicsWorld->addCollisionObject(obj.get());
    cc.collisionObject = std::move(obj);
}

void PhysicsWorld::onDestroyCollisionComponent(entt::registry& reg, entt::entity entity)
{
    auto& cc = reg.get<components::Collision>(entity);
    m_dynamicsWorld->removeCollisionObject(cc.collisionObject.get());
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

    const auto& indices = mesh.getIndices();
    const auto& vIn = mesh.getVertices();

    std::vector<XMFLOAT3> vOut(indices.size());

    for (size_t i = 0; i < indices.size(); i++) {
        vOut[i] = vIn[indices[i]].Position;
    }

    auto shape = std::make_unique<btConvexHullShape>(&vOut[0].x, int(vOut.size()),
        int(sizeof(XMFLOAT3)));

    shape->optimizeConvexHull();
    shape->initializePolyhedralFeatures();

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

void PhysicsWorld::setDebugDrawMode(int mode)
{
    m_debugDraw.setDebugMode(mode);
}

void PhysicsWorld::render()
{
    Im3d::PushMatrix(Im3d::Mat4(1.0f));
    m_dynamicsWorld->debugDrawWorld();
    Im3d::PopMatrix();
}

RaycastHit PhysicsWorld::raycast(math::WorldVector from0, math::WorldVector to0)
{
    btVector3 from, to;
    from.set128(from0.vec);
    to.set128(to0.vec);

    btCollisionWorld::ClosestRayResultCallback result(from, to);
    m_dynamicsWorld->rayTest(from, to, result);

    if (result.hasHit()) {
        RaycastHit hit;

        btVector3 h(0.1f, 0.1f, 0.1f);

        hit.entity = getEntity(result.m_collisionObject);
        hit.position.vec = XMVectorSetW(result.m_hitPointWorld.get128(), 1.0f);
        hit.normal.vec = XMVectorSetW(result.m_hitNormalWorld.get128(), 0.0f);

        return hit;
    }

    return RaycastHit{};
}

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
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

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString)
{
}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString)
{
}

void PhysicsDebugDraw::setDebugMode(int debugMode)
{
    m_debugMode = debugMode;
}

int PhysicsDebugDraw::getDebugMode() const
{
    return m_debugMode;
}
