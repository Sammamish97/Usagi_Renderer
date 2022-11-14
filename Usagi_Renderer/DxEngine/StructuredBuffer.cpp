#include "pch.h"
#include "StructuredBuffer.h"

#include "DxEngine.h"
#include "ResourceStateTracker.h"

#include "d3dx12.h"

StructuredBuffer::StructuredBuffer(const std::wstring& name)
    : Buffer(name)
    , m_CounterBuffer(
        CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        1, 4, name + L" Counter")
    , m_NumElements(0)
    , m_ElementSize(0)
{
}

StructuredBuffer::StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc,
    size_t numElements, size_t elementSize,
    const std::wstring& name)
    : Buffer(resDesc, numElements, elementSize, name)
    , m_CounterBuffer(
        CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        1, 4, name + L" Counter")
    , m_NumElements(numElements)
    , m_ElementSize(elementSize)
{
  
}