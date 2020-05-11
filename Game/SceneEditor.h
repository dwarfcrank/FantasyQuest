#pragma once

#include "DebugDraw.h"
#include "Game.h"

#include <entt/entt.hpp>

struct Scene;

class SceneEditor : public GameBase
{
public:
    SceneEditor(Scene& scene, InputMap& inputs);

    virtual bool update(float dt) override;
    virtual void render(class IRenderer*) override;
    virtual const Camera& getCamera() const override;

    DebugDraw d;

private:
    Scene& m_scene;

    Camera m_camera = Camera::perspective();

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;
    bool moveCamera = false;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    entt::entity m_currentEntity = entt::null;
    int m_currentObjectIdx = -1;

    void objectList();
    void objectPropertiesWindow();
    void lightList();
    void sceneWindow();

    void drawGrid();
};

