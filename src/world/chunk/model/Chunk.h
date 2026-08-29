#pragma once

#include <array>
#include <world/block/Block.h>

namespace OpenGLSomethingFrameDisplayerEVO
{
    constexpr int CHUNK_WIDTH = 16;
    constexpr int CHUNK_LENGTH = 16;
    constexpr int CHUNK_HEIGHT = 30;
    constexpr int CHUNK_TOTAL_BLOCKS = CHUNK_WIDTH * CHUNK_LENGTH * CHUNK_HEIGHT; // 7680

    static constexpr int GetIndex(const int x, const int z, const int y) noexcept
    {
        return (x * CHUNK_LENGTH + z) * CHUNK_HEIGHT + y;
    }

    class Chunk
    {
        std::array<Block, CHUNK_TOTAL_BLOCKS> m_blocks;
    public:
        int x = 0, z = 0;
        Chunk() = default;
        ~Chunk() = default;

        inline Block* GetBlock(int x, int z, int y) noexcept
        {
            if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_LENGTH || y < 0 || y >= CHUNK_HEIGHT)
                return nullptr;
            return &m_blocks[GetIndex(x, z, y)];
        }

        inline const Block* GetBlock(int x, int z, int y) const noexcept
        {
            if (x < 0 || x >= CHUNK_WIDTH || z < 0 || z >= CHUNK_LENGTH || y < 0 || y >= CHUNK_HEIGHT)
                return nullptr;
            return &m_blocks[GetIndex(x, z, y)];
        }

        inline void SetBlock(int x, int z, int y, const Block& block)
        {
            if (x >= 0 && x < CHUNK_WIDTH && z >= 0 && z < CHUNK_LENGTH && y >= 0 && y < CHUNK_HEIGHT)
                m_blocks[GetIndex(x, z, y)] = block;
        }

        inline const std::array<Block, CHUNK_TOTAL_BLOCKS>& GetBlocks() const noexcept
        {
            return m_blocks;
        }
    };
}
