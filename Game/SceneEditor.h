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
    ModelAsset(const std::string& name, Renderable* renderable, Bounds bounds, const std::string_view filename = "") :
        name(name), renderable(renderable), bounds(bounds), filename(filename) {}

    ModelAsset() = default;

    std::string name;
    std::string filename;
    Renderable* renderable = nullptr;
    Bounds bounds;
};

class SceneEditor : public GameBase
{
public:
    SceneEditor(Scene& scene, InputMap& inputs, const std::vector<ModelAsset>& models);

    virtual bool update(float dt) override;
    virtual void render(class IRenderer*) override;
    virtual const Camera& getCamera() const override;

    bool physicsEnabled() const
    {
        return m_physicsEnabled;
    }

private:
    Scene& m_scene;

    std::vector<ModelAsset> m_models;
    size_t m_currentModelIdx = 0;

    Camera m_camera = Camera::perspective();

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;
    bool moveCamera = false;
    bool m_physicsEnabled = false;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    bool entitySelected() const;

    entt::entity m_currentEntity = entt::null;
    int m_nextId = 0;

    void modelList();

    void entityList();
    void entityPropertiesWindow();
    void sceneWindow();

    entt::entity createEntity();

    void mainMenu();

    void drawGrid();

    void drawEntityBounds(entt::entity e);
};

