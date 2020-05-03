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

    const Camera& getCamera() const
    {
        return cam;
    }

private:
    Scene& scene;

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;

    Camera cam = Camera::perspective();

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 lightVelocity{ 0.0f, 0.0f, 0.0f };

    InputMap& inputs;
};

