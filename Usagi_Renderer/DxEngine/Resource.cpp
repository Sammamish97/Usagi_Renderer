#include "pch.h"
#include "Resource.h"

#include "d3dx12.h"
#include "DxEngine.h"
#include "DxUtil.h"
#include "ResourceStateTracker.h"

Resource::Resource(const std::wstring& name)
	: mName(name)
	, mFormatSupport({})
{
}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue,
	const std::wstring& name)
{
    if (clearValue)
    {
        mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }

    auto device = DxEngine::Get().GetDevice();

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        mClearValue.get(),
        IID_PPV_ARGS(&mResource)
    ));

    ResourceStateTracker::AddGlobalResourceState(mResource.Get(), D3D12_RESOURCE_STATE_COMMON);

    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(ComPtr<ID3D12Resource> resource, const std::wstring& name)
    : mResource(resource)
    , mFormatSupport({})
{
    CheckFeatureSupport();
    SetName(name);
}

Resource::Resource(const Resource& copy)
    : mResource(copy.mResource)
    , mFormatSupport(copy.mFormatSupport)
    , mName(copy.mName)
    , mClearValue(std::make_unique<D3D12_CLEAR_VALUE>(*copy.mClearValue))
{
}

Resource::Resource(Resource&& copy)
    : mResource(std::move(copy.mResource))
    , mFormatSupport(copy.mFormatSupport)
    , mName(std::move(copy.mName))
    , mClearValue(std::move(copy.mClearValue))
{
}

Resource& Resource::operator=(const Resource& other)
{
    if (this != &other)
    {
        mResource = other.mResource;
        mFormatSupport = other.mFormatSupport;
        mName = other.mName;
        if (other.mClearValue)
        {
            mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*other.mClearValue);
        }
    }
    return *this;
}

Resource& Resource::operator=(Resource&& other) noexcept
{
    if (this != &other)
    {
        mResource = std::move(other.mResource);
        mFormatSupport = other.mFormatSupport;
        mName = std::move(other.mName);
        mClearValue = std::move(other.mClearValue);

        other.Reset();
    }
    return *this;
}

Resource::~Resource()
{
}

void Resource::SetD3D12Resource(ComPtr<ID3D12Resource> d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
    mResource = d3d12Resource;
    if (mClearValue)
    {
        mClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
    else
    {
        mClearValue.reset();
    }
    CheckFeatureSupport();
    SetName(mName);
}

void Resource::SetName(const std::wstring& name)
{
    mName = name;
    if (mResource && !mName.empty())
    {
        mResource->SetName(mName.c_str());
    }
}

void Resource::Reset()
{
    mResource.Reset();
    mFormatSupport = {};
    mClearValue.reset();
    mName.clear();
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
    return (mFormatSupport.Support1 & formatSupport) != 0;
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
    return (mFormatSupport.Support2 & formatSupport) != 0;
}

void Resource::CheckFeatureSupport()
{
    if (mResource)
    {
        auto desc = mResource->GetDesc();
        auto device = DxEngine::Get().GetDevice();

        mFormatSupport.Format = desc.Format;
        ThrowIfFailed(device->CheckFeatureSupport(
            D3D12_FEATURE_FORMAT_SUPPORT,
            &mFormatSupport,
            sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
    }
    else
    {
        mFormatSupport = {};
    }
}
