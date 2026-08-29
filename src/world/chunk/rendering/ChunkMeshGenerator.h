#pragma once

#include <renderer/Mesh.h>
#include <world/chunk/model/Chunk.h>
#include <world/chunk/rendering/ChunkGraphicalData.h>

namespace OpenGLSomethingFrameDisplayerEVO
{
    class ChunkMeshGenerator
    {
    public:
        static ChunkGraphicalData GetChunkGraphicalData(const Chunk& chunk, const Chunk* left, const Chunk* right,
                                                        const Chunk* front, const Chunk* back);
    };
}
