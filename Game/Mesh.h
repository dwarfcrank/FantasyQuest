#pragma once

#include "Common.h"
#include "Renderer.h"

#include <filesystem>
#include <vector>

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

private:
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
};

