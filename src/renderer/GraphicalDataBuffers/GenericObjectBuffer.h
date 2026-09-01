#pragma once

#include <utility>
#include <mutex>
#include <stack>
#include <vector>
#include <libs/robin_hood.h>
#include <GL/glew.h>
#include <iostream>
#include <type_traits>

namespace OpenGLSomethingFrameDisplayerEVO
{
    template <typename T>
    struct ObjectBufferItem
    {
        T data;
        bool isDeleted = true;

        ObjectBufferItem() = default;

        explicit ObjectBufferItem(T&& data) : data(std::move(data)), isDeleted(false)
        {
        }
    };

    // Структура для отложенной записи
    template <typename T>
    struct PendingObjectWrite
    {
        T data;
        GLuint id{};

        PendingObjectWrite() = default;

        PendingObjectWrite(T&& data, GLuint id) : data(std::move(data)), id(id)
        {
        }
    };

    /*
     * GenericObjectBuffer - потокобезопасный буфер для хранения объектов любого типа
     * с поддержкой отложенной записи и удаления через ID.
     *
     * Особенности:
     * - Автоматическое переиспользование ID удаленных объектов
     * - Отложенная запись и удаление (актуализация через LoadData())
     * - Потокобезопасность через mutex
     * - Итераторы для обхода только активных объектов
     */
    template <typename T>
    class GenericObjectBuffer
    {
    private:
        using PendingWritesVector = std::vector<PendingObjectWrite<T>>;
        using ItemsVector = std::vector<ObjectBufferItem<T>>;
        std::stack<GLuint> m_tempIds;
    public:
        // Конструктор с возможностью указать начальную емкость
        explicit GenericObjectBuffer(size_t initialCapacity = 6222)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_items.reserve(initialCapacity);
            m_pendingWrites.reserve(initialCapacity);
            m_pendingIds.reserve(initialCapacity);

            m_items.resize(initialCapacity);

            // Предварительное заполнение стека свободных ID
            for (int i = static_cast<int>(initialCapacity) - 1; i >= 1; --i)
                m_freeIds.push(i);
            m_maxId = static_cast<GLuint>(initialCapacity);
        }

        // Запрет копирования
        GenericObjectBuffer(const GenericObjectBuffer&) = delete;
        GenericObjectBuffer& operator=(const GenericObjectBuffer&) = delete;

        // Перемещение
        GenericObjectBuffer(GenericObjectBuffer&& other) noexcept
            : m_items(std::move(other.m_items))
              , m_pendingWrites(std::move(other.m_pendingWrites))
              , m_pendingIds(std::move(other.m_pendingIds))
              , m_freeIds(std::move(other.m_freeIds))
              , m_maxId(other.m_maxId)
        {
        }

        // Запись объекта в буфер (отложенная операция)
        GLuint Write(T data)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            GLuint id;
            bool found = false;

            // 1. Сначала проверяем свободные ID
            if (!m_freeIds.empty())
            {
                id = m_freeIds.top();
                m_freeIds.pop();
                found = true;
            }
            // 2. Если свободных нет, берем из удаленных (они уже не используются)
            else if (!m_pendingDeletions.empty())
            {
                id = m_pendingDeletions.top();
                m_pendingDeletions.pop();
                found = true;
            }
            // 3. Если и там пусто, создаем новый
            else
            {
                id = m_maxId++;
                if (id >= m_items.size())
                    m_items.resize(id + 1);
            }

            m_pendingWrites.emplace_back(std::move(data), id);
            m_pendingIds.insert(id);
            return id;
        }


        // Пометить объект на удаление (отложенная операция)
        void DeleteAt(GLuint id)
        {
            std::lock_guard lock(m_mutex);
            if (id == 0) return;

            if (id < m_items.size() && !m_items[id].isDeleted)
            {
                m_pendingDeletions.push(id);
                //std::cout << "d: " << id << std::endl;
            }
        }

        // Очистить все данные
        void Clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& item : m_items)
            {
                item.data.Delete();
            }
            m_items.clear();

            m_pendingWrites.clear();
            m_pendingIds.clear();

            while (!m_pendingDeletions.empty())
                m_pendingDeletions.pop();
            while (!m_freeIds.empty())
                m_freeIds.pop();

            m_maxId = 0;

            constexpr auto initialCapacity = 62222;

            m_items.reserve(initialCapacity);
            m_pendingWrites.reserve(initialCapacity);
            m_pendingIds.reserve(initialCapacity);

            m_items.resize(initialCapacity);

            // Предварительное заполнение стека свободных ID
            for (int i = static_cast<int>(initialCapacity) - 1; i >= 1; --i)
                m_freeIds.push(i);
            m_maxId = static_cast<GLuint>(initialCapacity);
        }

        // Применить все отложенные операции (запись и удаление)
        std::vector<ObjectBufferItem<T>>& LoadData()
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            // Обработка удалений
            // В LoadData() при обработке удалений
            while (!m_pendingDeletions.empty())
            {
                GLuint id = m_pendingDeletions.top();
                m_pendingDeletions.pop();

                if (id < m_items.size() && !m_items[id].isDeleted)
                {
                    m_items[id].data.Delete();
                   // std::cout << "d: " << id << std::endl;
                    m_items[id].isDeleted = true;

                    // Возвращаем в freeIds ТОЛЬКО если этот ID не используется в pendingWrites
                    bool isUsedInPending = false;
                    for (const auto& pending : m_pendingWrites)
                    {
                        if (pending.id == id)
                        {
                            isUsedInPending = true;
                            break;
                        }
                    }
                    if (!isUsedInPending)
                    {
                        m_freeIds.push(id);
                    }
                }
            }

            // Обработка записей
            for (auto& pending : m_pendingWrites)
            {
                GLuint id = pending.id;

                // Расширяем вектор если нужно
                if (id >= m_items.size())
                    m_items.resize(id + 1);

                // ID мог быть переиспользован до обработки удаления (Write забрал его из
                // pendingDeletions, пока объект ещё жив). Освобождаем слот до перезаписи,
                // иначе старые VAO/VBO и данные утекают каждый кадр.
                if (!m_items[id].isDeleted)
                    m_items[id].data.Delete();
                m_items[id].isDeleted = true;

                // Если объект имеет метод Create - вызываем его
                // if constexpr (requires { pending.data.Create(); })
                {
                    pending.data.Create();
                    // std::cout << "а: " << id << std::endl;
                }

                // Перемещаем данные
                m_items[id].data = std::move(pending.data);
                m_items[id].isDeleted = false;
            }

            m_pendingWrites.clear();
            m_pendingIds.clear();

            return m_items;
        }

        // Получить данные (автоматически загружает отложенные операции)
        std::vector<ObjectBufferItem<T>>& GetData()
        {
            return LoadData();
        }

        // Получить данные без загрузки (только для чтения)
        const std::vector<ObjectBufferItem<T>>& GetData() const
        {
            return m_items;
        }

        // Получить объект по ID (без загрузки)
        T* GetObject(GLuint id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (id < m_items.size() && !m_items[id].isDeleted)
                return &m_items[id].data;
            return nullptr;
        }

        // Проверить существование объекта
        bool HasObject(GLuint id) const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return id < m_items.size() && !m_items[id].isDeleted;
        }

        // Количество активных объектов (только для чтения)
        size_t CountActive() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            size_t count = 0;
            for (const auto& item : m_items)
            {
                if (!item.isDeleted)
                    count++;
            }
            return count;
        }

        // Количество ожидающих записи
        size_t PendingCount() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_pendingWrites.size();
        }

        // Итератор для обхода только активных объектов
        class Iterator
        {
        public:
            Iterator(ItemsVector& items, size_t index)
                : m_items(items)
                  , m_index(index)
            {
                SkipDeleted();
            }

            ObjectBufferItem<T>& operator*() { return m_items[m_index]; }
            ObjectBufferItem<T>* operator->() { return &m_items[m_index]; }

            Iterator& operator++()
            {
                ++m_index;
                SkipDeleted();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator& other) const
            {
                return m_index == other.m_index;
            }

            bool operator!=(const Iterator& other) const
            {
                return m_index != other.m_index;
            }

        private:
            ItemsVector& m_items;
            size_t m_index;

            void SkipDeleted()
            {
                while (m_index < m_items.size() && m_items[m_index].isDeleted)
                {
                    ++m_index;
                }
            }
        };

        Iterator begin()
        {
            auto& items = LoadData();
            return Iterator(items, 0);
        }

        Iterator end()
        {
            auto& items = LoadData();
            return Iterator(items, items.size());
        }

        // Константный итератор
        class ConstIterator
        {
        public:
            ConstIterator(const ItemsVector& items, size_t index)
                : m_items(items)
                  , m_index(index)
            {
                SkipDeleted();
            }

            const ObjectBufferItem<T>& operator*() const { return m_items[m_index]; }
            const ObjectBufferItem<T>* operator->() const { return &m_items[m_index]; }

            ConstIterator& operator++()
            {
                ++m_index;
                SkipDeleted();
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const ConstIterator& other) const
            {
                return m_index == other.m_index;
            }

            bool operator!=(const ConstIterator& other) const
            {
                return m_index != other.m_index;
            }

        private:
            const ItemsVector& m_items;
            size_t m_index;

            void SkipDeleted()
            {
                while (m_index < m_items.size() && m_items[m_index].isDeleted)
                {
                    ++m_index;
                }
            }
        };

        ConstIterator begin() const
        {
            return ConstIterator(m_items, 0);
        }

        ConstIterator end() const
        {
            return ConstIterator(m_items, m_items.size());
        }

    private:
        ItemsVector m_items;
        PendingWritesVector m_pendingWrites;
        robin_hood::unordered_flat_set<GLuint> m_pendingIds;

        std::stack<GLuint> m_freeIds; // Стек свободных ID для переиспользования
        std::stack<GLuint> m_pendingDeletions; // Стек ID на удаление
        GLuint m_maxId = 0;
        mutable std::mutex m_mutex;
    };
} // namespace OpenGLSomethingFrameDisplayerEVO
