#pragma once

#include "DebugDraw.h"

struct Scene;

class SceneEditor
{
public:
    SceneEditor(Scene& scene);

    void update(float dt);

    void render(class IRenderer*);

    DebugDraw d;

private:
    Scene& m_scene;

    int m_currentObjectIdx = -1;

    void objectList();
    void objectPropertiesWindow();
    void lightList();
    void sceneWindow();
};

