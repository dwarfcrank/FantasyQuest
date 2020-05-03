#pragma once

#include "Camera.h"

#include <DirectXMath.h>

struct Scene;
class InputMap;

class Game
{
public:
    Game(Scene& scene, InputMap& inputs);

    bool update(float dt);
    void render(class Renderer& r);

private:
    Scene& scene;

    Camera cam;

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 lightVelocity{ 0.0f, 0.0f, 0.0f };

    InputMap& inputs;
};

