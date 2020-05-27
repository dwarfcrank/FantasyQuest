#pragma once

#include "Common.h"
#include "Renderer.h"
#include "Math.h"

#include <filesystem>
#include <vector>
#include <string>
#include <cereal/access.hpp>

class Mesh
{
public:
    struct SubMesh
    {
        u32 baseVertex = 0;
        u32 baseIndex = 0;
        u32 numIndices = 0;
        std::string material = "default";

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(baseVertex, baseIndex, numIndices, material);
        }
    };

    Mesh() = default;

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

    void setName(std::string_view name)
    {
        m_name = name;
    }

    const Bounds& getBounds() const
    {
        return m_bounds;
    }

    const std::vector<SubMesh>& getSubMeshes() const
    {
        return m_subMeshes;
    }

    static Mesh import(const std::filesystem::path& path);

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path);

private:
    friend class cereal::access;

    template<typename Archive>
    void serialize(Archive& archive)
    {
        archive(m_name, m_indices, m_vertices, m_bounds.max.vec, m_bounds.min.vec, m_subMeshes);
    }

    Bounds m_bounds;
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::vector<SubMesh> m_subMeshes;
    std::string m_name;
};

