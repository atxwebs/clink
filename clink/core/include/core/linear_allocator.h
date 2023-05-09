// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class LinearAllocator
{
public:
                          LinearAllocator(int size);
                          LinearAllocator(void* buffer, int size);
                          ~LinearAllocator();
    void*                 alloc(int size);
    template <class T> T* calloc(int count=1);

private:
    char*                 m_buffer;
    int                   m_used;
    int                   m_max;
    bool                  m_owned;
};

//------------------------------------------------------------------------------
inline LinearAllocator::LinearAllocator(int size)
: m_buffer((char*)malloc(size))
, m_used(0)
, m_max(size)
, m_owned(true)
{
}

//------------------------------------------------------------------------------
inline LinearAllocator::LinearAllocator(void* buffer, int size)
: m_buffer((char*)buffer)
, m_used(0)
, m_max(size)
, m_owned(false)
{
}

//------------------------------------------------------------------------------
inline LinearAllocator::~LinearAllocator()
{
    if (m_owned)
        free(m_buffer);
}

//------------------------------------------------------------------------------
inline void* LinearAllocator::alloc(int size)
{
    if (size == 0)
        return nullptr;

    int used = m_used + size;
    if (used > m_max)
        return nullptr;

    char* ptr = m_buffer + m_used;
    m_used = used;
    return ptr;
}

//------------------------------------------------------------------------------
template <class T> T* LinearAllocator::calloc(int count)
{
    return (T*)(alloc(sizeof(T) * count));
}
