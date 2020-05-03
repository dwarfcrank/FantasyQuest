#pragma once

struct Scene;
class Renderer;

class SceneEditor
{
public:
    SceneEditor(Scene& scene);

    void update(float dt);

    void render(Renderer&);

private:
    Scene& m_scene;

    void objectList();
    void lightList();
    void sceneWindow();
};

