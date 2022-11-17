#include "pch.h"
#include "DescriptorHeap.h"
#include "DxEngine.h"
DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorSize, UINT maxDescriptors)
	:mHeapType(heapType), mDescriptorSize(descriptorSize), mOffset(0), mMaxDescriptor(maxDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc;
	HeapDesc.NumDescriptors = mMaxDescriptor;
	HeapDesc.Type = mHeapType;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}
	HeapDesc.NodeMask = 0;
	DxEngine::Get().GetDevice()->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(mDescriptorHeap.GetAddressOf()));
}

UINT DescriptorHeap::GetDescriptorNum()
{
	return mOffset;
}

UINT DescriptorHeap::GetMaxDescriptors()
{
	return mMaxDescriptor;
}

//Allocate and return offset of descriptor.
UINT DescriptorHeap::GetNextAvailableIndex()
{
	return mOffset++;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT idx) const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), (INT)idx, mDescriptorSize);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT idx) const
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), (INT)idx, mDescriptorSize);
}

ComPtr<ID3D12DescriptorHeap>& DescriptorHeap::GetDescriptorHeap()
{
	return mDescriptorHeap;
}