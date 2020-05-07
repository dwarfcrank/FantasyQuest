#pragma once

#include "Camera.h"

#include <DirectXMath.h>

struct Scene;
class InputMap;

class GameBase
{
public:
    virtual ~GameBase() = default;

    virtual bool update(float dt) = 0;
    virtual void render(class IRenderer* r) = 0;

    virtual const Camera& getCamera() const = 0;

protected:
    GameBase(InputMap& inputs);

    InputMap& m_inputs;
};

class Game : public GameBase
{
public:
    Game(Scene& scene, InputMap& inputs);

    virtual bool update(float dt) override;
    virtual void render(class IRenderer* r) override;
    virtual const Camera& getCamera() const override;

private:
    Scene& scene;
    Camera m_camera = Camera::perspective();

    float moveSpeed = 5.1f;
    float turnSpeed = 3.0f;
    float angle = 0.0f;

    XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    XMFLOAT3 lightVelocity{ 0.0f, 0.0f, 0.0f };
};

