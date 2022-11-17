#pragma once
#include <d3d12.h>
#include <memory>
#include <wrl.h>

#include <string>

using Microsoft::WRL::ComPtr;

class Resource
{
public:
    explicit Resource(const std::wstring& name = L"");
    explicit Resource(const D3D12_RESOURCE_DESC& resourceDesc,
        const D3D12_CLEAR_VALUE* clearValue = nullptr,
        const std::wstring& name = L"");
    explicit Resource(ComPtr<ID3D12Resource> resource, const std::wstring& name = L"");

    Resource(const Resource& copy);
    Resource(Resource&& copy);

    Resource& operator=(const Resource& other);
    Resource& operator=(Resource&& other) noexcept;

    virtual ~Resource();

    bool IsValid() const
    {
        return (mResource != nullptr);
    }

    // Get access to the underlying D3D12 resource
    ComPtr<ID3D12Resource> GetD3D12Resource() const
    {
        return mResource;
    }

    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if (mResource)
        {
            resDesc = mResource->GetDesc();
        }
        return resDesc;
    }

    virtual void SetD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource,
        const D3D12_CLEAR_VALUE* clearValue = nullptr);

    void SetName(const std::wstring& name);
    virtual void Reset();
    bool CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const;
    bool CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const;
protected:
    // The underlying D3D12 resource.
    ComPtr<ID3D12Resource> mResource;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT mFormatSupport;
    std::unique_ptr<D3D12_CLEAR_VALUE> mClearValue;
    std::wstring mName;

private:
    // Check the format support and populate the mFormatSupport structure.
    void CheckFeatureSupport();
};

