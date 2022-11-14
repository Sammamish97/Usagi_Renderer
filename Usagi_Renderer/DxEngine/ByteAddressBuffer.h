#pragma once
#include <string>
#include "d3dx12.h"
#include "Buffer.h"

class ByteAddressBuffer : public Buffer
{
public:
    ByteAddressBuffer(const std::wstring& name = L"");
    ByteAddressBuffer(const D3D12_RESOURCE_DESC& resDesc,
        size_t numElements, size_t elementSize,
        const std::wstring& name = L"");

    size_t GetBufferSize() const
    {
        return m_BufferSize;
    }

private:
    size_t m_BufferSize;
};

