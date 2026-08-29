//
// Created by dmitry on 21.08.2026.
//

#include "BlockGPUDataArray.h"
#include <GlobalVectorPool.h>

namespace OpenGLSomethingFrameDisplayerEVO
{
    BlockGPUDataArray::BlockGPUDataArray()
    = default;

    BlockGPUDataArray::~BlockGPUDataArray()
    = default;

    BlockGPUDataArray::BlockGPUDataArray(BlockGPUDataArray&& other) noexcept
        : m_VAO(std::move(other.m_VAO)),
          m_VBO(std::move(other.m_VBO)),
          m_blocks(std::move(other.m_blocks))
    {
    }

    BlockGPUDataArray& BlockGPUDataArray::operator=(BlockGPUDataArray&& other) noexcept
    {
        if (this != &other)
        {
            m_VAO = std::move(other.m_VAO);
            m_VBO = std::move(other.m_VBO);
            m_blocks = std::move(other.m_blocks);
        }
        return *this;
    }

    void BlockGPUDataArray::Create()
    {
        m_VAO.Create();
        m_VBO.Create();
    }

    void BlockGPUDataArray::SetData(std::vector<BlockGPUData> blocks)
    {
        m_blocks = std::move(blocks);
    }

    void BlockGPUDataArray::Setup()
    {
        m_VAO.Activate();
        m_VBO.SetData(m_blocks, GL_DYNAMIC_DRAW);
        // position (ivec3)
        m_VAO.AttribIPointer(0, 3, AttribType::Int, sizeof(BlockGPUData), offsetof(BlockGPUData, position));
        // texCoords[0..3] (uvec4)
        m_VAO.AttribIPointer(1, 4, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, texCoords));
        // texCoords[4..7] (uvec4)
        m_VAO.AttribIPointer(2, 4, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, texCoords) + 4);
        // texCoords[8..11] (uvec4)
        m_VAO.AttribIPointer(3, 4, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, texCoords) + 8);
        // geometryIndex (uint)
        m_VAO.AttribIPointer(4, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, geometryIndex));
        // flags (uint)
        m_VAO.AttribIPointer(5, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, flags));
        // visibleFaces (uint)
        m_VAO.AttribIPointer(6, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, visibleFaces));
        m_VAO.Deactivate();
    }

    void BlockGPUDataArray::Draw()
    {
        m_VAO.Activate();
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_blocks.size()));
        m_VAO.Deactivate();
    }

    void BlockGPUDataArray::Delete()
    {
        m_VAO.Delete();
        m_VBO.Delete();
        m_blocks.clear();
    }
}