#include "pch.h"
#include "IndexBuffer.h"
#include <cassert>

IndexBuffer::IndexBuffer(const std::wstring& name)
    : Buffer(name)
    , m_NumIndicies(0)
    , m_IndexFormat(DXGI_FORMAT_UNKNOWN)
    , m_IndexBufferView({})
{}

IndexBuffer::~IndexBuffer()
{}

void IndexBuffer::CreateIndexBufferView(size_t numElements, size_t elementSize)
{
    assert(elementSize == 2 || elementSize == 4 && "Indices must be 16, or 32-bit integers.");

    m_NumIndicies = numElements;
    m_IndexFormat = (elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_IndexBufferView.BufferLocation = mResource->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = static_cast<UINT>(numElements * elementSize);
    m_IndexBufferView.Format = m_IndexFormat;
}
