#include "pch.h"
#include "CommandList.h"
#include "DxEngine.h"
#include <cassert>
#include "RenderTarget.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "Texture.h"
#include "ByteAddressBuffer.h"
#include "ConstantBuffer.h"
#include "CommandQueue.h"
#include "StructuredBuffer.h"
#include "UploadBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "DirectXTex.h"
#include <filesystem>

#include "d3dx12.h"
#include "DxUtil.h"


std::map<std::wstring, ID3D12Resource* > CommandList::ms_TextureCache;
std::mutex CommandList::ms_TextureCacheMutex;
using namespace DirectX;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
    : m_d3d12CommandListType(type)
{
    auto device = DxEngine::Get().GetDevice();

    ThrowIfFailed(device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));

    ThrowIfFailed(device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(),
        nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));

    m_UploadBuffer = std::make_unique<UploadBuffer>();

    m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();
}

CommandList::~CommandList()
{}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
    if (resource)
    {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subresource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
    TransitionBarrier(resource.GetD3D12Resource(), stateAfter, subresource, flushBarriers);
}

void CommandList::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());

    m_ResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
    UAVBarrier(resource.GetD3D12Resource());
}

void CommandList::AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> beforeResource, Microsoft::WRL::ComPtr<ID3D12Resource> afterResource, bool flushBarriers)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(beforeResource.Get(), afterResource.Get());

    m_ResourceStateTracker->ResourceBarrier(barrier);

    if (flushBarriers)
    {
        FlushResourceBarriers();
    }
}

void CommandList::AliasingBarrier(const Resource& beforeResource, const Resource& afterResource, bool flushBarriers)
{
    AliasingBarrier(beforeResource.GetD3D12Resource(), afterResource.GetD3D12Resource());
}

void CommandList::FlushResourceBarriers()
{
    m_ResourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::ClearTexture(const Texture& texture, D3D12_CPU_DESCRIPTOR_HANDLE cpuDesc, const float clearColor[4])
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_d3d12CommandList->ClearRenderTargetView(cpuDesc, clearColor, 0, nullptr);

    TrackResource(texture);
}

void CommandList::ClearDepthStencilTexture(const Texture& texture, D3D12_CPU_DESCRIPTOR_HANDLE cpuDesc, D3D12_CLEAR_FLAGS clearFlags, float depth, uint8_t stencil)
{
    TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    m_d3d12CommandList->ClearDepthStencilView(cpuDesc, clearFlags, depth, stencil, 0, nullptr);

    TrackResource(texture);
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes)
{
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

    TrackResource(dstRes);
    TrackResource(srcRes);
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
    CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());
}

void CommandList::ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource, uint32_t srcSubresource)
{
    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

    FlushResourceBarriers();

    m_d3d12CommandList->ResolveSubresource(dstRes.GetD3D12Resource().Get(), dstSubresource, srcRes.GetD3D12Resource().Get(), srcSubresource, dstRes.GetD3D12ResourceDesc().Format);

    TrackResource(srcRes);
    TrackResource(dstRes);
}


void CommandList::CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    auto device = DxEngine::Get().GetDevice();

    size_t bufferSize = numElements * elementSize;

    ComPtr<ID3D12Resource> d3d12Resource;
    if (bufferSize == 0)
    {
        // This will result in a NULL resource (which may be desired to define a default null resource).
    }
    else
    {
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&d3d12Resource)));

        // Add the resource to the global resource state tracker.
        ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

        if (bufferData != nullptr)
        {
            // Create an upload resource to use as an intermediate buffer to copy the buffer resource 
            ComPtr<ID3D12Resource> uploadResource;
            ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&uploadResource)));

            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = bufferData;
            subresourceData.RowPitch = bufferSize;
            subresourceData.SlicePitch = subresourceData.RowPitch;

            m_ResourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
            FlushResourceBarriers();

            UpdateSubresources(m_d3d12CommandList.Get(), d3d12Resource.Get(),
                uploadResource.Get(), 0, 0, 1, &subresourceData);

            // Add references to resources so they stay in scope until the command list is reset.
            TrackResource(uploadResource);
        }
        TrackResource(d3d12Resource);
    }

    buffer.SetD3D12Resource(d3d12Resource);
}

void CommandList::CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
    CopyBuffer(vertexBuffer, numVertices, vertexStride, vertexBufferData);
}

void CommandList::CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    CopyBuffer(indexBuffer, numIndicies, indexSizeInBytes, indexBufferData);
}

void CommandList::CopyByteAddressBuffer(ByteAddressBuffer& byteAddressBuffer, size_t bufferSize, const void* bufferData)
{
    CopyBuffer(byteAddressBuffer, 1, bufferSize, bufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void CommandList::CopyStructuredBuffer(StructuredBuffer& structuredBuffer, size_t numElements, size_t elementSize, const void* bufferData)
{
    CopyBuffer(structuredBuffer, numElements, elementSize, bufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    m_d3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::LoadTextureFromFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage)
{
    std::filesystem::path filePath(fileName);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found.");
    }

    std::lock_guard<std::mutex> lock(ms_TextureCacheMutex);
    auto iter = ms_TextureCache.find(fileName);
    if (iter != ms_TextureCache.end())
    {
        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(iter->second);
        texture.SetName(fileName);
    }
    else
    {
        TexMetadata metadata;
        ScratchImage scratchImage;

        if (filePath.extension() == ".dds")
        {
            ThrowIfFailed(LoadFromDDSFile(
                fileName.c_str(),
                DDS_FLAGS_NONE,
                &metadata,
                scratchImage));
        }
        else if (filePath.extension() == ".hdr")
        {
            ThrowIfFailed(LoadFromHDRFile(
                fileName.c_str(),
                &metadata,
                scratchImage));
        }
        else if (filePath.extension() == ".tga")
        {
            ThrowIfFailed(LoadFromTGAFile(
                fileName.c_str(),
                &metadata,
                scratchImage));
        }
        else
        {
            ThrowIfFailed(LoadFromWICFile(
                fileName.c_str(),
                WIC_FLAGS_NONE,
                &metadata,
                scratchImage));
        }

        D3D12_RESOURCE_DESC textureDesc = {};
        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE1D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE2D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.arraySize));
            break;
        case TEX_DIMENSION_TEXTURE3D:
            textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(
                metadata.format,
                static_cast<UINT64>(metadata.width),
                static_cast<UINT>(metadata.height),
                static_cast<UINT16>(metadata.depth));
            break;
        default:
            throw std::exception("Invalid texture dimension.");
            break;
        }

        auto device = DxEngine::Get().GetDevice();
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&textureResource)));

        texture.SetTextureUsage(textureUsage);
        texture.SetD3D12Resource(textureResource);
        texture.SetName(fileName);

        // Update the global state tracker.
        ResourceStateTracker::AddGlobalResourceState(
            textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

        std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
        const Image* pImages = scratchImage.GetImages();
        for (int i = 0; i < scratchImage.GetImageCount(); ++i)
        {
            auto& subresource = subresources[i];
            subresource.RowPitch = pImages[i].rowPitch;
            subresource.SlicePitch = pImages[i].slicePitch;
            subresource.pData = pImages[i].pixels;
        }

        CopyTextureSubresource(
            texture,
            0,
            static_cast<uint32_t>(subresources.size()),
            subresources.data());

        // Add the texture resource to the texture cache.
        ms_TextureCache[fileName] = textureResource.Get();
    }
}

void CommandList::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
    auto device = DxEngine::Get().GetDevice();
    auto destinationResource = texture.GetD3D12Resource();

    if (destinationResource)
    {
        // Resource must be in the copy-destination state.
        TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
        FlushResourceBarriers();

        UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), firstSubresource, numSubresources);

        // Create a temporary (intermediate) resource for uploading the subresources
        ComPtr<ID3D12Resource> intermediateResource;
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateResource)
        ));

        UpdateSubresources(m_d3d12CommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

        TrackResource(intermediateResource);
        TrackResource(destinationResource);
    }
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetComputeDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes,
	const void* bufferData)
{
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetComputeRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
    m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer)
{
    TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    auto vertexBufferView = vertexBuffer.GetVertexBufferView();

    m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

    TrackResource(vertexBuffer);
}

void CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
    size_t bufferSize = numVertices * vertexSize;

    auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, vertexSize);
    memcpy(heapAllocation.CPU, vertexBufferData, bufferSize);

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    vertexBufferView.BufferLocation = heapAllocation.GPU;
    vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

    m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void CommandList::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
    TransitionBarrier(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

    auto indexBufferView = indexBuffer.GetIndexBufferView();

    m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);

    TrackResource(indexBuffer);
}

void CommandList::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
    size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
    size_t bufferSize = numIndicies * indexSizeInBytes;

    auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, indexSizeInBytes);
    memcpy(heapAllocation.CPU, indexBufferData, bufferSize);

    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    indexBufferView.BufferLocation = heapAllocation.GPU;
    indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
    indexBufferView.Format = indexFormat;

    m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void CommandList::SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData)
{
    size_t bufferSize = numElements * elementSize;

    auto heapAllocation = m_UploadBuffer->Allocate(bufferSize, elementSize);

    memcpy(heapAllocation.CPU, bufferData, bufferSize);

    m_d3d12CommandList->SetGraphicsRootShaderResourceView(slot, heapAllocation.GPU);
}
void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
    SetViewports({ viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
    assert(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()),
        viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
    SetScissorRects({ scissorRect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
    assert(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
    m_d3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()),
        scissorRects.data());
}

void CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
    m_d3d12CommandList->SetPipelineState(pipelineState.Get());

    TrackResource(pipelineState);
}

void CommandList::SetGraphicsRootSignature(ComPtr<ID3D12RootSignature> rootSignature)
{
    m_d3d12CommandList->SetGraphicsRootSignature(rootSignature.Get());

    TrackResource(rootSignature);//Maybe do not need.
}

void CommandList::SetComputeRootSignature(ComPtr<ID3D12RootSignature> rootSignature)
{
    m_d3d12CommandList->SetComputeRootSignature(rootSignature.Get());

    TrackResource(rootSignature);//Maybe do not need.
}


void CommandList::SetSingleRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle)
{
    m_d3d12CommandList->OMSetRenderTargets(1,rtvHandle, FALSE, dsvHandle);
}

void CommandList::SetComputeRootSRV(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
    m_d3d12CommandList->SetComputeRootShaderResourceView(rootIndex, gpuAddress);
}

void CommandList::SetComputeRootUAV(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
    m_d3d12CommandList->SetComputeRootUnorderedAccessView(rootIndex, gpuAddress);
}

void CommandList::SetDescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap)
{
    m_d3d12CommandList->SetDescriptorHeaps(1, heap.GetAddressOf());
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
    FlushResourceBarriers();

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
    FlushResourceBarriers();
    m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
    FlushResourceBarriers();
    m_d3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

bool CommandList::Close(CommandList& pendingCommandList)
{
    // Flush any remaining barriers.
    FlushResourceBarriers();

    m_d3d12CommandList->Close();

    // Flush pending resource barriers.
    uint32_t numPendingBarriers = m_ResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
    // Commit the final resource state to the global state.
    m_ResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
}

void CommandList::Close()
{
    FlushResourceBarriers();
    m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
    ThrowIfFailed(m_d3d12CommandAllocator->Reset());
    ThrowIfFailed(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

    m_ResourceStateTracker->Reset();
    m_UploadBuffer->Reset();

    ReleaseTrackedObjects();

    m_RootSignature = nullptr;
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
    m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(const Resource& res)
{
    TrackResource(res.GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects()
{
    m_TrackedObjects.clear();
}