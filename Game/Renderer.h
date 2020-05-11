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
#include <string_view>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class Camera;

struct Bounds
{
    Vector<Model> min;
    Vector<Model> max;
};

class Renderable;
struct Transform;

struct RenderBatch
{
    Renderable* renderable;
    std::vector<RenderableConstants> instances;
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void setDirectionalLight(const XMFLOAT3& pos, const XMFLOAT3& color) = 0;
    virtual void setPointLights(ArrayView<PointLight> lights) = 0;
    virtual void draw(Renderable*, const Camera&, const struct Transform&) = 0;
    virtual void draw(const RenderBatch& batch, const Camera&) = 0;
    virtual void debugDraw(const Camera&, ArrayView<DebugDrawVertex>) = 0;
    virtual void clear(float r, float g, float b) = 0;

    virtual void draw(const RenderBatch& batch) = 0;
    virtual void drawShadow(const RenderBatch& batch) = 0;

    virtual void beginFrame() = 0;

    virtual void beginFrame(const Camera&) = 0;
    virtual void endFrame() = 0;

    virtual void beginShadowPass() = 0;
    virtual void drawShadow(Renderable*, const Camera&, const struct Transform&) = 0;

    virtual void beginShadowPass(const Camera&) = 0;
    virtual void endShadowPass() = 0;

    virtual void postProcess(const std::array<float, 6>&) = 0;

    virtual Renderable* createRenderable(std::string_view name, ArrayView<Vertex> vertices, ArrayView<u16> indices) = 0;

    virtual void initImgui() = 0;
    virtual void drawImgui() = 0;
};

std::unique_ptr<IRenderer> createRenderer(SDL_Window*);

