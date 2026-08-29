#include "ChunkMeshGenerator.h"
#include "world/chunk/rendering/ChunkGraphicalData.h"
#include <GlobalVectorPool.h>
#include <iostream>
#include <world/block/BlockMeshGenerator.h>

namespace OpenGLSomethingFrameDisplayerEVO
{
    // Порядок граней: back, front, top, bottom, left, right
    static const std::array<std::string, 6> FACE_NAMES = {
        "back", "front", "top", "bottom", "left", "right"
    };

    ChunkGraphicalData ChunkMeshGenerator::GetChunkGraphicalData(
        const Chunk& chunk,
        const Chunk* left, const Chunk* right,
        const Chunk* front, const Chunk* back)
    {
        ChunkGraphicalData graphicalData = {};
        std::vector<BlockGPUData>& blocksData = graphicalData.allBlockTypesMeshData.blocksGPUData;
        blocksData.reserve(CHUNK_LENGTH * CHUNK_WIDTH * CHUNK_HEIGHT);

        for (int x = 0; x < CHUNK_LENGTH; ++x)
        {
            for (int z = 0; z < CHUNK_WIDTH; ++z)
            {
                for (int y = 0; y < CHUNK_HEIGHT; ++y)
                {
                    const auto block = chunk.GetBlock(x, z, y);

                    if (!block || !block->blockInfo)
                        continue;

                    // вроде так и надо. потому что
                    // мы делаем видимые грани уже на cpu
                    // и тем более шейдер изза vbo не ебет ничего о соседних блоках
                    // соответственно  о блоках воздуха
                    // ему знать необязательно
                    if (block->blockInfo->geometry_type[0] == 'a')
                        continue;

                    BlockGPUData blockGPUData = {};
                    blockGPUData.position = {x, y, z};
                    blockGPUData.flags = block->blockInfo->is_transparent ? 1 : 0;
                    blockGPUData.geometryIndex = static_cast<uint8_t>(block->blockInfo->geometry_type[0]);


                    int texX = 0, texZ = 0;
                    int i = 0;

                    for (const auto& blockFace : block->blockInfo->faces)
                    {
                        {
                            texX = blockFace.texture_x;
                            texZ = blockFace.texture_z;
                            blockGPUData.texCoords[i * 2] = static_cast<uint8_t>(texX);
                            blockGPUData.texCoords[i * 2 + 1] = static_cast<uint8_t>(texZ);
                            i++;
                        }
                    }

                    // Определяем соседей
                    const Block* neighbors[6] = {nullptr};

                    // back (0)
                    if (x > 0)
                        neighbors[0] = chunk.GetBlock(x - 1, z, y);
                    else if (left)
                        neighbors[0] = left->GetBlock(CHUNK_LENGTH - 1, z, y);

                    // front (1)
                    if (x < CHUNK_LENGTH - 1)
                        neighbors[1] = chunk.GetBlock(x + 1, z, y);
                    else if (right)
                        neighbors[1] = right->GetBlock(0, z, y);

                    // top (2)
                    if (y < CHUNK_HEIGHT - 1)
                        neighbors[2] = chunk.GetBlock(x, z, y + 1);

                    // bottom (3)
                    if (y > 0)
                        neighbors[3] = chunk.GetBlock(x, z, y - 1);

                    // left (4)
                    if (z > 0)
                        neighbors[4] = chunk.GetBlock(x, z - 1, y);
                    else if (back)
                        neighbors[4] = back->GetBlock(x, CHUNK_WIDTH - 1, y);

                    // right (5)
                    if (z < CHUNK_WIDTH - 1)
                        neighbors[5] = chunk.GetBlock(x, z + 1, y);
                    else if (front)
                        neighbors[5] = front->GetBlock(x, 0, y);

                    // Определяем видимые грани
                    for (int i = 0; i < 6; i++)
                    {
                        if (const auto neighbor = neighbors[i]; !neighbor || !neighbor->blockInfo || neighbor->blockInfo->is_transparent)
                        {
                            blockGPUData.visibleFaces |= (1 << i);
                        }
                    }

                    blocksData.push_back(blockGPUData);
                }
            }
        }

        return graphicalData;
    }
}