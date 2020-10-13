#pragma once

#include "Game.h"
#include "Renderer.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include <unordered_map>
#include <functional>

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

// HACK: This exception is thrown when we want to load a new scene, so we catch the
// exception and destroy/rebuild everything
struct LoadSceneException
{
    std::filesystem::path path;
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

    using ComponentEditorFunc = void (SceneEditor::*)(entt::entity);
    std::unordered_map<entt::id_type, ComponentEditorFunc> m_componentEditors;

    std::vector<ModelAsset> m_models;

    int m_currentModelIdx = -1;

    Camera m_camera = Camera::perspective({ 1920.0f, 1080.0f });

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;
    bool moveCamera = false;
    bool m_physicsEnabled = false;
    bool m_drawPhysics = false;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };

    bool entitySelected() const;

    entt::entity m_currentEntity = entt::null;
    int m_nextId = 0;

    void modelList();

    void entityList();
    void entityPropertiesWindow();

    void renderableComponentEditor(entt::entity);
    void physicsComponentEditor(entt::entity);
    void collisionComponentEditor(entt::entity);
    void pointLightComponentEditor(entt::entity);

    class btCollisionShape* getCollisionMesh(const std::string&);

    void sceneWindow();

    entt::entity createEntity();

    void mainMenu();

    void drawGrid();

    void drawEntityBounds(entt::entity e);
};

