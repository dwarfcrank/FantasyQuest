#pragma once

#include "Common.h"

#include "ArrayView.h"
#include "ShaderCommon.h"

#include "Math.h"

#include <im3d.h>
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
    math::Vector<math::Model> min;
    math::Vector<math::Model> max;
};

class Renderable;
struct Transform;

struct RenderBatch
{
    Renderable* renderable;
    std::vector<RenderableConstants> instances;
};

struct PostProcessParams
{
    float exposure = 1.0f;
    bool gammaCorrection = false;
    float deltaTime = 0.0f;
};

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void setDirectionalLight(const XMFLOAT3& pos, const XMFLOAT3& color, float intensity) = 0;
    virtual void setPointLights(ArrayView<PointLight> lights) = 0;
    virtual void drawIm3d(const Camera&, ArrayView<Im3d::DrawList>) = 0;
    virtual void clear(float r, float g, float b) = 0;

    virtual void beginFrame(const Camera&) = 0;
    virtual void draw(const RenderBatch& batch) = 0;
    virtual void endFrame() = 0;

    virtual void beginShadowPass(const Camera&) = 0;
    virtual void drawShadow(const RenderBatch& batch) = 0;
    virtual void endShadowPass() = 0;

    virtual void postProcess(const PostProcessParams&) = 0;

    virtual Renderable* createRenderable(std::string_view name, ArrayView<Vertex> vertices, ArrayView<u16> indices) = 0;
    virtual Renderable* createRenderable(const class Mesh&) = 0;

    virtual void initImgui() = 0;
    virtual void drawImgui() = 0;
};

std::unique_ptr<IRenderer> createRenderer(SDL_Window*);

