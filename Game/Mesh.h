#pragma once

#include "Common.h"
#include "Renderer.h"
#include "Math.h"

#include <filesystem>
#include <vector>
#include <string>
#include <cereal/access.hpp>

struct MeshFileHeader
{
    u32 magic;
    u32 headerSize;
    u32 nameLength;
    u32 numVertices;
    u32 numIndices;
    float minBounds[3];
    float maxBounds[3];

    static constexpr u32 MAGIC = 0x214d5146; // little endian "FQM!"
};

class Mesh
{
public:
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

    static Mesh import(const std::filesystem::path& path);

    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path);

private:
    friend class cereal::access;

    template<typename Archive>
    void serialize(Archive& archive)
    {
        archive(m_name, m_indices, m_vertices, m_bounds.max.vec, m_bounds.min.vec);
    }

    Bounds m_bounds;
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::string m_name;
};

