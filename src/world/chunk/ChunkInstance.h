#pragma once

#include <memory>
#include <vector>
#include <utility>

#include "model/Chunk.h"
#include "rendering/ChunkGraphicalData.h"
#include "world/block/Block.h"
#include <renderer/Mesh.h>

namespace OpenGLSomethingFrameDisplayerEVO
{
    using VertexDataPair = std::pair<std::vector<Vertex>, std::vector<GLuint>>;

    class ChunkInstance
    {
        ChunkGraphicalData m_graphicalData;
        std::unique_ptr<Chunk> m_chunk;
        bool m_isMeshDataValid = false;

    public:
        ChunkInstance(int x, int z);
        ~ChunkInstance();
        void SetBlock(int x, int y, int z, const Block& block);
        Block* GetBlock(int x, int y, int z) const;
        int GetChunkX() const { return m_chunk->x; }
        int GetChunkZ() const { return m_chunk->z; }

        GLuint GetMeshID() const { return m_graphicalData.allBlockTypesMeshData.IDinBuffer; }
        void SetMeshID(const GLuint id) { m_graphicalData.allBlockTypesMeshData.IDinBuffer = id; }

        ChunkGraphicalData& GetGraphicalData();
        bool IsMeshValid() const { return m_isMeshDataValid; }
        void InvalidateMeshData() { m_isMeshDataValid = false; }
        void GenerateGraphicalDataIfNeeded(ChunkInstance* left, ChunkInstance* right, ChunkInstance* front,
                                           ChunkInstance* back);
    };
}
