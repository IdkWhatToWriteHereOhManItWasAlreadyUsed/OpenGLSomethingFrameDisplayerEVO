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
        // Position (ivec3)
        m_VAO.AttribPointer(0, 3, AttribType::Int, sizeof(BlockGPUData), offsetof(BlockGPUData, position));
        // texCoords (uint8_t[12]) - передаём как массив из 12 байт
        m_VAO.AttribPointer(1, 12, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, texCoords));
        // geometryIndex (uint8_t)
        m_VAO.AttribPointer(2, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, geometryIndex));
        // flags (uint8_t)
        m_VAO.AttribPointer(3, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, flags));
        // visibleFaces (uint8_t)
        m_VAO.AttribPointer(4, 1, AttribType::UnsignedByte, sizeof(BlockGPUData), offsetof(BlockGPUData, visibleFaces));
        m_VAO.Deactivate();
    }

    void BlockGPUDataArray::Draw()
    {
        m_VAO.DrawArrays(0, m_blocks.size());
    }

    void BlockGPUDataArray::Delete()
    {
        m_VAO.Delete();
        m_VBO.Delete();
        m_blocks.clear();
    }
}