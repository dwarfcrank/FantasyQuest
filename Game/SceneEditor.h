#pragma once

#include "Game.h"
#include "Renderer.h"

#include <entt/entt.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

struct Scene;

struct ModelAsset
{
    ModelAsset(const std::string& name, Renderable* renderable, Bounds bounds) :
        name(name), renderable(renderable), bounds(bounds) {}

    ModelAsset() = default;

    std::string name;
    Renderable* renderable = nullptr;
    Bounds bounds;
};

class SceneEditor : public GameBase
{
public:
    SceneEditor(Scene& scene, InputMap& inputs, const std::vector<ModelAsset>& renderables);

    virtual bool update(float dt) override;
    virtual void render(class IRenderer*) override;
    virtual const Camera& getCamera() const override;

private:
    Scene& m_scene;

    std::vector<ModelAsset> m_renderables;
    size_t m_currentModelIdx = 0;

    Camera m_camera = Camera::perspective();

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;
    bool moveCamera = false;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    entt::entity m_currentEntity = entt::null;

    void modelList();

    void entityList();
    void entityPropertiesWindow();
    void lightList();
    void sceneWindow();

    entt::entity createEntity();

    void mainMenu();

    void drawGrid();
};

