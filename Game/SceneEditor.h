#pragma once

#include "DebugDraw.h"
#include "Game.h"

#include <entt/entt.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

struct Scene;

class Renderable;

class SceneEditor : public GameBase
{
public:
    SceneEditor(Scene& scene, InputMap& inputs, const std::unordered_map<std::string, Renderable*>& renderables);

    virtual bool update(float dt) override;
    virtual void render(class IRenderer*) override;
    virtual const Camera& getCamera() const override;

    DebugDraw d;

private:
    Scene& m_scene;

    std::vector<std::tuple<std::string, Renderable*>> m_renderables;
    size_t m_lastUsedRenderable = 0;

    Camera m_camera = Camera::perspective();

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;
    bool moveCamera = false;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    entt::entity m_currentEntity = entt::null;
    int m_currentObjectIdx = -1;

    void renderableList();

    void entityList();
    void entityPropertiesWindow();
    void lightList();
    void sceneWindow();

    entt::entity createEntity();

    void mainMenu();

    void drawGrid();
};

