#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

class CommandQueue;
class IDemo;
class Window;

class DxEngine
{
public:
	static void Create(HINSTANCE hInst);
	static DxEngine& Get();
	static void Destroy();

public:
	std::shared_ptr<Window> CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true);
	void DestroyWindow(std::shared_ptr<Window> window);
	std::shared_ptr<Window> GetWindowByName(const std::wstring& windowName);
	int Run(std::shared_ptr<IDemo> pGame);
	void Quit(int exitCode = 0);
	void Flush();

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
	static uint64_t GetFrameCount()
	{
		return msFrameCount;
	}

public:
	bool IsTearingSupported() const;
	DXGI_SAMPLE_DESC GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) const;
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	ComPtr<ID3D12Device2> GetDevice() const;

public:
	void CreateRtvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
	void CreateDsvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
	void CreateCbvDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuLocation, size_t bufferSize, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
	void CreateSrvDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);
	void CreateUavDescriptor(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos);

protected:
	DxEngine(HINSTANCE hInst);
	virtual ~DxEngine();
	void Initialize();
	ComPtr<IDXGIAdapter4> GetAdapter(bool bUseWarp);
	ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
	bool CheckTearingSupport();

private:
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	DxEngine(const DxEngine& other) = delete;
	DxEngine& operator=(const DxEngine& other) = delete;

	HINSTANCE mHinstance;

	ComPtr<ID3D12Device2> mDevice;

	std::shared_ptr<CommandQueue> mDirectCommandQueue;
	std::shared_ptr<CommandQueue> mComputeCommandQueue;
	std::shared_ptr<CommandQueue> mCopyCommandQueue;

	bool mTearingSupported;

	static uint64_t msFrameCount;
};

