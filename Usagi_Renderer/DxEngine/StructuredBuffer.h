#pragma once
#include "Buffer.h"

#include "ByteAddressBuffer.h"

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(const std::wstring& name = L"");
    StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc,
        size_t numElements, size_t elementSize,
        const std::wstring& name = L"");

    virtual size_t GetNumElements() const
    {
        return m_NumElements;
    }

    virtual size_t GetElementSize() const
    {
        return m_ElementSize;
    }

private:
    size_t m_NumElements;
    size_t m_ElementSize;
};

