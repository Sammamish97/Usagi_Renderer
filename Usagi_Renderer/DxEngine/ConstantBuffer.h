#pragma once
#include "Buffer.h"
class ConstantBuffer : public Buffer
{
public:
    ConstantBuffer(const std::wstring& name = L"");
    virtual ~ConstantBuffer();
    
    size_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }

private:
    size_t m_SizeInBytes;
};