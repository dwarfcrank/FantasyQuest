#pragma once

#include "Common.h"
#include "Renderer.h"
#include "Math.h"

#include <filesystem>
#include <vector>
#include <string>

class Mesh
{
public:
    Mesh(const std::filesystem::path& filename);

    const std::vector<Vertex>& getVertices() const
    {
        return m_vertices;
    }

    const std::vector<u16>& getIndices() const
    {
        return m_indices;
    }

    const std::string& getName() const
    {
        return m_name;
    }

    const Bounds& getBounds() const
    {
        return m_bounds;
    }

private:
    Bounds m_bounds;
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::string m_name;
};

