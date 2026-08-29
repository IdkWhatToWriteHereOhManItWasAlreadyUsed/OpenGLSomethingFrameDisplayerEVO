#pragma once

#include <GL/glew.h>
#include <utility>
#include <vector>
#include <renderer/Mesh.h>
#include <renderer/BlockGPUDataArray.h>
#include <array>

namespace OpenGLSomethingFrameDisplayerEVO
{
    using VertexDataPair = std::pair<std::vector<Vertex>, std::vector<GLuint>>;
    using AABB = std::pair<glm::vec3, glm::vec3>;

    enum class GeometryType: std::size_t
    {
        Solid,
        Transparent,
        //Liquid,
        //
        Count
    };

    struct RawChunkMeshData
    {
        VertexDataPair Data;
        GLuint IDinBuffer = 0;
    };

    struct RawBlocksGPUAlignedData
    {
        std::vector<BlockGPUData> blocksGPUData;
        GLuint IDinBuffer = 0;
    };

    struct GPUReadyChunkMeshData
    {
        Mesh mesh;
        AABB aabb;
        VertexDataPair rawChunkVertexDataForMesh;

        GPUReadyChunkMeshData() = default;

        GPUReadyChunkMeshData(AABB aabb, VertexDataPair vertexData)
            : aabb(std::move(aabb)), rawChunkVertexDataForMesh(std::move(vertexData))
        {
        }

        void Create()
        {
            mesh.Create();
            mesh.SetData(std::move(rawChunkVertexDataForMesh.first), std::move(rawChunkVertexDataForMesh.second));
            mesh.Setup();
        }

        void Delete()
        {
            mesh.Delete();
        }
    };

    struct GPUReadyChunkGPUAlignedData
    {
        std::vector<BlockGPUData> rawBlocksGPUData;
        BlockGPUDataArray blockGPUDataArray;

        GPUReadyChunkGPUAlignedData() = default;

        void Create()
        {
            blockGPUDataArray.Create();
            blockGPUDataArray.SetData(std::move(rawBlocksGPUData));
            blockGPUDataArray.Setup();
        }

        void Delete()
        {
            blockGPUDataArray.Delete();
        }
    };

    struct ChunkGraphicalData
    {
        RawBlocksGPUAlignedData allBlockTypesMeshData = {};
    };
}
