#include "pch.h"
#include "Mesh.h"

#include "Math.h"
#include "File.h"
#include "Serialization.h"
#include "ShaderCommon.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <fstream>

#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>

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

template<typename Archive>
void serialize(Archive& archive, Vertex& v)
{
    archive(v.Position, v.Normal, v.Color, v.Texcoord);
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

    result.m_subMeshes.resize(scene->mNumMeshes);

    u16 baseIdx = 0;

    for (u32 meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        const auto* mesh = scene->mMeshes[meshIdx];
        auto& submesh = result.m_subMeshes[meshIdx];

        submesh.baseIndex = baseIdx;
        submesh.baseVertex = static_cast<u32>(result.m_vertices.size());
        {
            const auto& name = scene->mMaterials[mesh->mMaterialIndex]->GetName();
            submesh.material.assign(name.data, name.length);
        }

        aabbMin = min3(aabbMin, mesh->mAABB.mMin);
        aabbMax = max3(aabbMax, mesh->mAABB.mMax);

        for (u32 i = 0; i < mesh->mNumVertices; i++) {
            const auto& v = mesh->mVertices[i];
            const auto& n = mesh->mNormals[i];
            const auto& color = materialColors[mesh->mMaterialIndex];

            XMFLOAT2 texcoord(0.0f, 0.0f);

            if (mesh->HasTextureCoords(0)) {
                const auto& t = mesh->mTextureCoords[0][i];
                texcoord.x = t.x;
                texcoord.y = t.y;
            }

            result.m_vertices.push_back(Vertex{
                .Position{ v.x, v.y, v.z },
                .Normal{ n.x, n.y, n.z },
                .Color{ color.r, color.g, color.b, 1.0f },
                .Texcoord{ texcoord },
            });
        }

        for (ArrayView faces(mesh->mFaces, mesh->mNumFaces); const auto& f : faces) {
            for (ArrayView indices(f.mIndices, f.mNumIndices); auto i : indices) {
                result.m_indices.push_back(baseIdx + static_cast<u16>(i));
                submesh.numIndices++;
            }
        }

        baseIdx = static_cast<u16>(result.m_indices.size());
    }

    // TODO: the kenney assets are built to work in a grid, so undoing the local
    // offsets makes the pieces hard to align - need to figure this out
    if constexpr (false) {
        aiVector3D center = aabbMin + ((aabbMax - aabbMin) * 0.5f);

        aabbMin -= center;
        aabbMax -= center;

        for (auto& vertex : result.m_vertices) {
            vertex.Position.x -= center.x;
            vertex.Position.y -= center.y;
            vertex.Position.z -= center.z;
        }
    }

    result.m_bounds.min = Vector<Model>(aabbMin.x, aabbMin.y, aabbMin.z, 1.0f);
    result.m_bounds.max = Vector<Model>(aabbMax.x, aabbMax.y, aabbMax.z, 1.0f);

    if (const auto& meshName = scene->mMeshes[0]->mName; meshName.length > 0) {
        result.m_name = scene->mMeshes[0]->mName.C_Str();
    } else {
        result.m_name = path.filename().generic_string();
    }

    if (result.m_name.starts_with("Mesh ")) {
        result.m_name = result.m_name.substr(5);
    }

    g_importer.FreeScene();

    return result;
}

void Mesh::load(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    cereal::BinaryInputArchive archive(input);
    archive(*this);
}

void Mesh::save(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary);
    cereal::BinaryOutputArchive archive(output);
    archive(*this);
}
