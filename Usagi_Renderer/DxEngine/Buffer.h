#pragma once
#include "Resource.h"
class Buffer : public Resource
{
public:
    explicit Buffer(const std::wstring& name = L"");
    explicit Buffer(const D3D12_RESOURCE_DESC& resDesc,
        size_t numElements, size_t elementSize,
        const std::wstring& name = L"");
};

