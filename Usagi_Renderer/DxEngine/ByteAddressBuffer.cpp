#include "pch.h"
#include "ByteAddressBuffer.h"
#include "DxEngine.h"

ByteAddressBuffer::ByteAddressBuffer(const std::wstring& name)
    : Buffer(name)
{
}

ByteAddressBuffer::ByteAddressBuffer(const D3D12_RESOURCE_DESC& resDesc,
    size_t numElements, size_t elementSize,
    const std::wstring& name)
    : Buffer(resDesc, numElements, elementSize, name)
{}

