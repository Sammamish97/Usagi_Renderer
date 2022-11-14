#pragma once
#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(const std::wstring& name = L"");
    virtual ~IndexBuffer();

    // Inherited from Buffer
    virtual void CreateIndexBufferView(size_t numElements, size_t elementSize);

    size_t GetNumIndicies() const
    {
        return m_NumIndicies;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return m_IndexFormat;
    }

    /**
     * Get the index buffer view for biding to the Input Assembler stage.
     */
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return m_IndexBufferView;
    }
    
private:
    size_t m_NumIndicies;
    DXGI_FORMAT m_IndexFormat;

    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};

