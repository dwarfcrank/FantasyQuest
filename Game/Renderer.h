#pragma once

#include "Common.h"

#include "DebugDraw.h"
#include "ArrayView.h"
#include "ShaderCommon.h"

#include <SDL2/SDL.h>
#include <fmt/format.h>
#include <type_traits>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include <tuple>
#include <functional>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Camera;

struct Bounds
{
    Vector<Model> min;
    Vector<Model> max;
};

class Renderable;

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void setDirectionalLight(const XMFLOAT3& pos) = 0;
    virtual void setPointLights(ArrayView<PointLight> lights) = 0;
    virtual void draw(Renderable*, const Camera&, const struct Transform&) = 0;
    virtual void debugDraw(const Camera&, ArrayView<DebugDrawVertex>) = 0;
    virtual void clear(float r, float g, float b) = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual void beginShadowPass() = 0;
    virtual void drawShadow(Renderable*, const Camera&, const struct Transform&) = 0;
    virtual void endShadowPass() = 0;

    virtual void fullScreenPass() = 0;

    virtual Renderable* createRenderable(ArrayView<Vertex> vertices, ArrayView<u16> indices) = 0;

    ID3D11Device1* getDevice() { return m_device.Get(); }
    ID3D11DeviceContext1* getDeviceContext() { return m_context.Get(); }

protected:
    ComPtr<ID3D11Device1> m_device;
    ComPtr<ID3D11DeviceContext1> m_context;
};

std::unique_ptr<IRenderer> createRenderer(SDL_Window*);

