#pragma once
#include <wrl.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;
enum class HeapType
{
	RTV,
	DSV,
	SRV_1D,
	SRV_2D,
	SRV_CUBE,
	UAV_2D,
	UAV_2D_ARRAY
};
class DescriptorHeap
{
public:
	DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorSize, UINT maxDescriptors);
	UINT GetNextAvailableIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT idx) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT idx) const;
	ComPtr<ID3D12DescriptorHeap>& GetDescriptorHeap();

	UINT GetDescriptorNum();
	UINT GetMaxDescriptors();

private:
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;

	UINT mDescriptorSize;
	UINT mMaxDescriptor;

	UINT mOffset;
};

