//
// Created by dmitry on 21.08.2026.
//

#ifndef BADPLAYER_BLOCKGPUDATAARRAY_H
#define BADPLAYER_BLOCKGPUDATAARRAY_H

#include "ArrayObject.h"
#include "BufferObject.h"
#include <vector>
#include "world/block/Block.h"

namespace OpenGLSomethingFrameDisplayerEVO
{

    class BlockGPUDataArray
    {
    public:
        BlockGPUDataArray();
        ~BlockGPUDataArray();

        BlockGPUDataArray(const BlockGPUDataArray&) = delete;
        BlockGPUDataArray& operator=(const BlockGPUDataArray&) = delete;

        BlockGPUDataArray(BlockGPUDataArray&& other) noexcept;
        BlockGPUDataArray& operator=(BlockGPUDataArray&& other) noexcept;

        void Create();
        void SetData(std::vector<BlockGPUData> blocks);
        void Setup();
        void Draw();
        void Delete();

        std::vector<BlockGPUData>& getBlocks() { return m_blocks; }

    private:
        ArrayObject m_VAO = ArrayObject();
        BufferObject m_VBO = BufferObject(BufferType::ArrayBuffer);
        // мб для них тоже имеет смысл замутить пул?
        std::vector<BlockGPUData> m_blocks;
    };
}

#endif //BADPLAYER_BLOCKGPUDATAARRAY_H