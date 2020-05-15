#pragma once

#include "Common.h"
#include "Renderer.h"
#include "Math.h"

#include <filesystem>
#include <vector>
#include <string>

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

    const Bounds& getBounds() const
    {
        return m_bounds;
    }

    static Mesh import(const std::filesystem::path& path);

    static Mesh load(const std::filesystem::path& path);
    static void save(const std::filesystem::path& path, const Mesh& mesh);

private:
    Bounds m_bounds;
    std::vector<Vertex> m_vertices;
    std::vector<u16> m_indices;
    std::string m_name;
};

