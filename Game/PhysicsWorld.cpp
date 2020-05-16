#include "pch.h"

#include "Transform.h"
#include "PhysicsWorld.h"
#include "im3d.h"
#include "Scene.h"

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
}

PhysicsWorld::~PhysicsWorld()
{
    for (auto&& obj : m_collisionObjects) {
        m_dynamicsWorld->removeCollisionObject(obj.get());
    }
}

void PhysicsWorld::onCreate(entt::registry& reg, entt::entity entity)
{
    auto& tc = reg.get<components::Transform>(entity);

    btCollisionShape* shape = nullptr;

    float mass = 5.0f;

    if (const auto* rc = reg.try_get<components::Renderable>(entity)) {
        XMFLOAT3 h;
        XMStoreFloat3(&h, ((rc->bounds.max - rc->bounds.min) * 0.5f).vec);
        auto volume = (h.x * h.y * h.z) * 8.0f;
        mass = volume / 10.0f;
        shape = m_collisionShapes.emplace_back(new btBoxShape(btVector3(h.x, h.y, h.z))).get();
    } else {
        float radius = 2.5f;
        auto volume = (4.0f / 3.0f) * XM_PI * radius * radius * radius;
        shape = m_collisionShapes.emplace_back(new btSphereShape(radius)).get();
    }

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(tc.position.x, tc.position.y, tc.position.z));

    btQuaternion rot;
    rot.setEuler(tc.rotation.y, tc.rotation.x, tc.rotation.z);
    transform.setRotation(rot);

    btVector3 localInertia(0.0f, 0.0f, 0.0f);
    shape->calculateLocalInertia(mass, localInertia);

    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState.get(), shape, localInertia);
    auto body = std::make_unique<btRigidBody>(info);

    body->setUserPointer((void*)(uintptr_t)entity);

    m_dynamicsWorld->addRigidBody(body.get());
    m_collisionObjects.emplace_back(std::move(body));
    m_motionStates.emplace_back(std::move(motionState));
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

void PhysicsWorld::update(float dt)
{
    m_time += dt;

    if (m_time < TIMESTEP) {
        return;
    }

    m_time -= TIMESTEP;

    m_dynamicsWorld->stepSimulation(TIMESTEP, 10);

    {
        Transform tt;
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
            XMFLOAT3 position(origin.x(), origin.y(), origin.z());

            XMFLOAT3 rotation;
            //ft.getRotation().getEulerZYX(rotation.z, rotation.y, rotation.x);
            ft.getRotation().getEulerZYX(rotation.z, rotation.y, rotation.x);

            tt.Position = XMLoadFloat3(&position);
            tt.Rotation = XMLoadFloat3(&rotation);

            auto entity = (entt::entity)(uintptr_t)obj->getUserPointer();

            if (m_scene.reg.valid(entity)) {
                // TODO: ideally we'd only have physics objects in entities
                m_scene.reg.patch<components::Transform>(entity, [&](components::Transform& tc) {
                    tc.position = position;
                    tc.rotation = rotation;
                });
            }

            XMFLOAT4X4 tt2;
            XMStoreFloat4x4(&tt2, tt.getMatrix());
            Im3d::Mat4 tm;
            std::memcpy(&tm, &tt2, sizeof(tm));

            const auto* shape = obj->getCollisionShape();

            Im3d::PushMatrix(tm);
            Im3d::PushSize(2.5f);
            Im3d::PushColor(Im3d::Color_Cyan);
            if (shape->getShapeType() == BOX_SHAPE_PROXYTYPE) {
                const auto* box = static_cast<const btBoxShape*>(shape);
                const auto halfExtents = box->getHalfExtentsWithoutMargin();

                Im3d::Vec3 h(halfExtents.x(), halfExtents.y(), halfExtents.z());
                Im3d::DrawAlignedBox({ -h.x, -h.y, -h.z }, h);
            }
            else if (shape->getShapeType() == SPHERE_SHAPE_PROXYTYPE) {
                const auto* sphere = static_cast<const btSphereShape*>(shape);
                Im3d::DrawSphere(Im3d::Vec3(0.0f), sphere->getRadius());
            }
            Im3d::PopColor();
            Im3d::PopSize();
            Im3d::PopMatrix();
        }
    }
}

void PhysicsWorld::render()
{
}
