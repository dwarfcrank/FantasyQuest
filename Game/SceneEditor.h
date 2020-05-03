#pragma once

struct Scene;

class SceneEditor
{
public:
    SceneEditor(Scene& scene);

    void update(float dt);

    void render(class IRenderer*);

private:
    Scene& m_scene;

    void objectList();
    void lightList();
    void sceneWindow();
};

