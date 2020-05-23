#pragma once

#include "../Renderer.h"

namespace components
{

struct Renderable
{
    Renderable(std::string name, ::Renderable* renderable, const Bounds& bounds) :
        name{ std::move(name) }, renderable{ renderable }, bounds{ bounds } {}

    Renderable() = default;

    std::string name;
    ::Renderable* renderable = nullptr;
    Bounds bounds{ math::Vector<math::Model>(), math::Vector<math::Model>() };
};

template<typename Archive>
void serialize(Archive& archive, Renderable& r)
{
    archive(r.name);
}

}
