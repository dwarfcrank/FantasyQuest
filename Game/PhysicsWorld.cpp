#include "pch.h"

#include "Transform.h"
#include "PhysicsWorld.h"
#include "im3d.h"
#include "Scene.h"
#include "Mesh.h"

#include <bullet/btBulletDynamicsCommon.h>

PhysicsWorld::PhysicsWorld(Scene& scene) :
    m_scene(scene)
{
    m_collisionConfiguration.reset(new btDefaultCollisionConfiguration());
    m_dispatcher.reset(new btCollisionDispatcher(m_collisionConfiguration.get()));
    m_overlappingPairCache.reset(new btDbvtBroadphase());
    m_solver.reset(new btSequentialImpulseConstraintSolver());
    m_dynamicsWorld.reset(new btDiscreteDynamicsWorld(
        m_dispatcher.get(), m_overlappingPairCache.get(), m_solver.get(), m_collisionConfiguration.get()));

    m_dynamicsWorld->setGravity(btVector3(0.0f, -10.0f, 0.0f));

    auto shape = m_collisionShapes.emplace_back(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f))).get();
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
    btTransform ft;

    auto n = m_dynamicsWorld->getNumCollisionObjects();
    const auto& collisionObjects = m_dynamicsWorld->getCollisionObjectArray();

    for (int i = 0; i < n; i++) {
        auto obj = collisionObjects[i];

        if (auto body = btRigidBody::upcast(obj)) {
            if (auto motionState = body->getMotionState()) {
                motionState->getWorldTransform(ft);
            }
        } else {
            ft = obj->getWorldTransform();
        }

        const auto& origin = ft.getOrigin();

        auto T = XMMatrixTranslationFromVector(ft.getOrigin().get128());
        auto R = XMMatrixRotationQuaternion(ft.getRotation().get128());
        auto S = XMMatrixScaling(1.0f, 1.0f, 1.0f);

        const auto* shape = obj->getCollisionShape();

        Im3d::PushMatrix(S * R * T);
        Im3d::PushSize(2.5f);
        Im3d::PushColor(Im3d::Color_Cyan);

        if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
            const auto* box = static_cast<const btBoxShape*>(shape);
            const auto halfExtents = box->getHalfExtentsWithoutMargin();

            Im3d::Vec3 h(halfExtents.x(), halfExtents.y(), halfExtents.z());
            Im3d::DrawAlignedBox({ -h.x, -h.y, -h.z }, h);
        } else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
            const auto* sphere = static_cast<const btSphereShape*>(shape);
            Im3d::DrawSphere(Im3d::Vec3(0.0f), sphere->getRadius());
        }

        Im3d::PopColor();
        Im3d::PopSize();
        Im3d::PopMatrix();
    }
}
