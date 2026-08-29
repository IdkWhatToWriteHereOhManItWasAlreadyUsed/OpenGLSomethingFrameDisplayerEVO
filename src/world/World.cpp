#include <ThreadPool/ThreadPool.h>
#include "GlobalThreadPool.h"
#include "chunk/rendering/RayCaster.h"
#include "World.h"
#include "chunk/generation/ChunkGenerator.h"
#include "block/Block.h"
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <renderer/GraphicalDataBuffers/GenericObjectBuffer.h>
#include "chunk/ChunkInstance.h"
#include "chunk/rendering/ChunkGraphicalData.h"
#include "world/chunk/model/Chunk.h"
#include <array>

namespace OpenGLSomethingFrameDisplayerEVO
{
    World::World()
    {
    }

    void World::Initialise(int width, int height)
    {
        m_width = width / CHUNK_WIDTH;
        m_height = height / CHUNK_LENGTH;

        m_loadedChunks.resize(m_width);
        for (auto& chunkLine : m_loadedChunks)
        {
            chunkLine.resize(m_height);
            for (auto& chunk : chunkLine)
            {
                chunk = std::make_unique<ChunkInstance>(0, 0);
            }
        }

        m_numThreads = globalThreadPool.GetThreadCount() * 2;
        m_idsToDelete.resize(m_numThreads);
        // m_blocksGraphicalData.resize(m_numThreads);
    }

    World::World(int seed)
    {
    }

    void World::SetSeed(int seed)
    {
    }

    World::~World()
    {
    }

    void World::Update(float playerX, float playerZ)
    {
        //UpdateChunks(playerX, playerZ);
    }

    const Block* World::GetBlock(int x, int y, int z) const
    {
        return nullptr;
    }


    void World::SetBlock(int x, int y, int z, Block block)
    {
        return;
    }

    RayCastResult World::GetBlockPlayerLooksAt(glm::vec3 rayStart, glm::vec3 rayDir, GLfloat distance) const
    {
        return RayCaster::castRay(rayStart, rayDir, this, distance);
    }

    void World::DisplayFrame(std::vector<std::vector<uint8_t>>& data)
    {
        m_numThreads = globalThreadPool.GetThreadCount();

        int rowsPerStrip = m_width / m_numThreads;
        int extraRows = m_width % m_numThreads;

        int currentX = 0;
        for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex)
        {
            int stripHeight = rowsPerStrip;
            if (stripIndex == m_numThreads - 1)
                stripHeight += extraRows;
            if (stripHeight == 0) continue;

            int endX = currentX + stripHeight;

            auto task = std::make_shared<Task<void, World*,
                                              std::vector<std::vector<uint8_t>>*,
                                              int, int, int>>
            (
                [](World* world,
                   std::vector<std::vector<uint8_t>>* dataPtr,
                   int start, int end, int threadIndex)
                {
                    world->GenerateStrip(*dataPtr, start, end, threadIndex);
                },
                this, &data, currentX, endX, stripIndex
            );

            m_threadPool->AddTask(task);
            currentX = endX;
        }

        m_threadPool->WaitAll();
        currentX = 0;

        for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex)
        {
            int stripHeight = rowsPerStrip;
            if (stripIndex == m_numThreads - 1)
                stripHeight += extraRows;
            if (stripHeight == 0) continue;

            int endX = currentX + stripHeight;

            auto task = std::make_shared<Task<void, World*, std::vector<std::vector<uint8_t>>*,
                                             int, int, int>>
            (
                [](World* world,
                   std::vector<std::vector<uint8_t>>* dataPtr,
                   int start, int end, int threadIndex)
                {
                    world->GenerateGPUArraysOfStrip(*dataPtr, start, end, threadIndex);
                },
                this, &data, currentX, endX, stripIndex
            );

            m_threadPool->AddTask(task);
            currentX = endX;
        }

        m_threadPool->WaitAll();
        currentX = 0;

        /*
            на удивление эта хрень без разделения на потоки пашет лучше, чем с разделением
            видимо я недооценил стоимость добавления тасок в тредпул
            ну это мб даже и логично было
            Боттлнек, связанный с сранием из кучи потоков в один буфер подтвердился
            на удивление, в одном потоке с его учетом фпс тоже выше получаеися
            а хотя это же логично, у меня в буферах ресайз маленький
            а может и нет
            ну, работает хорошо и так сойдет
        */
        
        std::lock_guard lock(m_worldMutex);
        for (int stripIndex = 0; stripIndex < m_numThreads; ++stripIndex)
        {
            int stripHeight = rowsPerStrip;
            if (stripIndex == m_numThreads - 1)
                stripHeight += extraRows;
            if (stripHeight == 0) continue;

            const int endX = currentX + stripHeight;

            for (int i = currentX; i < endX; i++)
            {
                for (const auto & checkedChunk : m_loadedChunks[i])
                {
                    auto& chunkGraphicalData = checkedChunk->GetGraphicalData();
                    m_blocksGraphicalData.DeleteAt (checkedChunk->GetMeshID());
                    GPUReadyChunkGPUAlignedData dataForBuffer;
                    dataForBuffer.blockGPUDataArray.SetData(
                        std::move(chunkGraphicalData.allBlockTypesMeshData.blocksGPUData)
                        );
                    checkedChunk->SetMeshID(m_blocksGraphicalData.Write(std::move(dataForBuffer)));
                }
                currentX = endX;
            }
        }
    }


    void World::GenerateStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX, int threadIndex)
    {
        for (int i = startX; i < endX; i++)
        {
            for (auto j = 0; j < m_loadedChunks[i].size(); j++)
            {
                auto newChunk = std::make_unique<ChunkInstance>(i, j);
                ChunkGenerator::GenerateChunk(*newChunk, data);
                newChunk->InvalidateMeshData();

                m_loadedChunks[i][j].reset();
                m_loadedChunks[i][j] = std::move(newChunk);
            }
        }
    }

    void World::GenerateGPUArraysOfStrip(std::vector<std::vector<uint8_t>>& data, int startX, int endX,
                                           int threadIndex) const
    {
        for (int i = startX; i < endX; i++)
        {
            for (auto j = 0; j < m_loadedChunks[i].size(); j++)
            {
                auto x = i;
                auto z = j;

                ChunkInstance* left = nullptr;
                ChunkInstance* right = nullptr;
                ChunkInstance* front = nullptr;
                ChunkInstance* back = nullptr;

                if (x - 1 >= 0)
                    left = m_loadedChunks[x - 1][z].get();

                if (z - 1 >= 0)
                    back = m_loadedChunks[x][z - 1].get();

                if (x + 1 < m_loadedChunks.size())
                    right = m_loadedChunks[x + 1][z].get();

                if (z + 1 < m_loadedChunks[x].size())
                    front = m_loadedChunks[x][z + 1].get();

                const GLuint idToDelete = m_loadedChunks[i][j]->GetGraphicalData().allBlockTypesMeshData.IDinBuffer;

                m_loadedChunks[x][z]->GenerateGraphicalDataIfNeeded(left, right, front, back);

                m_loadedChunks[x][z]->SetMeshID(idToDelete);
            }
        }
    }
}
