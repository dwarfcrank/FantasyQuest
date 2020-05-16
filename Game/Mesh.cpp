#include "pch.h"
#include "Mesh.h"

#include "Math.h"
#include "File.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <unordered_map>

using namespace math;

static Assimp::Importer g_importer;

template<typename T1, typename T2>
static T1 min3(const T1& a, const T2& b)
{
    T1 r;
    r.x = std::min(a.x, b.x);
    r.y = std::min(a.y, b.y);
    r.z = std::min(a.z, b.z);

    return r;
}

template<typename T1, typename T2>
static T1 max3(const T1& a, const T2& b)
{
    T1 r;
    r.x = std::max(a.x, b.x);
    r.y = std::max(a.y, b.y);
    r.z = std::max(a.z, b.z);

    return r;
}

Mesh Mesh::import(const std::filesystem::path& path)
{
    Mesh result;

    constexpr auto flags =
        aiProcess_CalcTangentSpace
        | aiProcess_GenBoundingBoxes
        | aiProcess_Triangulate
        //| aiProcess_GenSmoothNormals
        //| aiProcess_ForceGenNormals
        | aiProcess_ConvertToLeftHanded
        | aiProcess_SortByPType
        //| aiProcess_FixInfacingNormals
        | aiProcess_GenNormals
        | aiProcess_OptimizeMeshes
        | aiProcess_PreTransformVertices
        | aiProcess_ImproveCacheLocality
        ;

    auto scene = g_importer.ReadFile(path.generic_string(), flags);

    if (!scene) {
        throw std::runtime_error(fmt::format("Loading mesh {} failed!", path.generic_string()));
    }

    std::vector<aiColor3D> materialColors(scene->mNumMaterials);

    for (u32 i = 0; i < scene->mNumMaterials; i++) {
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, materialColors[i]);
    }

    auto aabbMin = scene->mMeshes[0]->mAABB.mMin;
    auto aabbMax = scene->mMeshes[0]->mAABB.mMax;

    // TODO: probably shouldn't just merge different meshes into one...
    u16 baseIdx = 0;

    for (ArrayView meshes(scene->mMeshes, scene->mNumMeshes); const auto* mesh : meshes) {
        aabbMin = min3(aabbMin, mesh->mAABB.mMin);
        aabbMax = max3(aabbMax, mesh->mAABB.mMax);

        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            const auto& v = mesh->mVertices[i];
            const auto& n = mesh->mNormals[i];
            const auto& color = materialColors[mesh->mMaterialIndex];

            result.m_vertices.push_back(Vertex{
                .Position{v.x, v.y, v.z},
                .Normal{n.x, n.y, n.z},
                .Color{color.r, color.g, color.b, 1.0f}
            });
        }

        for (ArrayView faces(mesh->mFaces, mesh->mNumFaces); const auto& f : faces) {
            for (ArrayView indices(f.mIndices, f.mNumIndices); auto i : indices) {
                result.m_indices.push_back(baseIdx + static_cast<u16>(i));
            }
        }

        baseIdx = static_cast<u16>(result.m_indices.size());
    }

    aiVector3D center = aabbMin + ((aabbMax - aabbMin) * 0.5f);

    aabbMin -= center;
    aabbMax -= center;

    for (auto& vertex : result.m_vertices) {
        vertex.Position.x -= center.x;
        vertex.Position.y -= center.y;
        vertex.Position.z -= center.z;
    }

    result.m_bounds.min = Vector<Model>(aabbMin.x, aabbMin.y, aabbMin.z, 1.0f);
    result.m_bounds.max = Vector<Model>(aabbMax.x, aabbMax.y, aabbMax.z, 1.0f);

    if (const auto& meshName = scene->mMeshes[0]->mName; meshName.length > 0) {
        result.m_name = scene->mMeshes[0]->mName.C_Str();
    } else {
        result.m_name = path.filename().generic_string();
    }

    g_importer.FreeScene();

    return result;
}

Mesh Mesh::load(const std::filesystem::path& path)
{
    auto data = loadFile(path);

    std::size_t offset = 0;
    MeshFileHeader header;

    std::memcpy(&header, &data[offset], sizeof(header));

    assert(header.magic == MeshFileHeader::MAGIC);
    assert(header.headerSize == sizeof(MeshFileHeader));

    offset += sizeof(header);

    Mesh result;

    result.m_bounds.min = Vector<Model>(header.minBounds[0], header.minBounds[1], header.minBounds[2], 1.0f);
    result.m_bounds.max = Vector<Model>(header.maxBounds[0], header.maxBounds[1], header.maxBounds[2], 1.0f);

    result.m_name.assign(&data[offset], &data[offset + header.nameLength]);
    offset += header.nameLength;

    result.m_vertices.resize(header.numVertices);
    std::memcpy(result.m_vertices.data(), &data[offset], header.numVertices * sizeof(Vertex));
    offset += header.numVertices * sizeof(Vertex);

    result.m_indices.resize(header.numIndices);
    std::memcpy(result.m_indices.data(), &data[offset], header.numIndices * sizeof(u16));
    offset += header.numIndices * sizeof(u16);

    return result;
}

void Mesh::save(const std::filesystem::path& path, const Mesh& mesh)
{
    XMFLOAT3 minB, maxB;

    XMStoreFloat3(&minB, mesh.m_bounds.min.vec);
    XMStoreFloat3(&maxB, mesh.m_bounds.max.vec);

    MeshFileHeader header{
        .magic = MeshFileHeader::MAGIC,
        .headerSize = u32(sizeof(MeshFileHeader)),
        .nameLength = u32(mesh.m_name.length()),
        .numVertices = u32(mesh.m_vertices.size()),
        .numIndices = u32(mesh.m_indices.size()),
        .minBounds{ minB.x, minB.y, minB.z },
        .maxBounds{ maxB.x, maxB.y, maxB.z },
    };

    std::ofstream o{ path, std::ios::binary };
    o.write(reinterpret_cast<const char*>(&header), sizeof(header));
    o.write(mesh.m_name.data(), header.nameLength);
    o.write(reinterpret_cast<const char*>(mesh.m_vertices.data()), header.numVertices * sizeof(Vertex));
    o.write(reinterpret_cast<const char*>(mesh.m_indices.data()), header.numIndices * sizeof(u16));
}

