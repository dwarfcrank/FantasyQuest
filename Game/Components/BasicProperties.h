#pragma once

#include <string>

namespace components
{

struct Misc
{
    Misc() = default;

    Misc(std::string name) :
        name{ std::move(name) }
    {
    }

    std::string name;
};

template<typename Archive>
void serialize(Archive& archive, Misc& m)
{
    archive(m.name);
}

}
