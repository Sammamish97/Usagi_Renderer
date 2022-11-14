// DxEngine.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "DxEngine.h"
#include "BufferFormat.h"
#include "Window.h"
#include "IDemo.h"

#include <cassert>

#include "DxUtil.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";
static std::shared_ptr<Window> gsWindow;
static DxEngine* gsSingleTon = nullptr;

uint64_t DxEngine::msFrameCount = 0;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// A wrapper struct to allow shared pointers for the window class.
// This is needed because the constructor and destructor for the Window
// class are protected and not accessible by the std::make_shared method.
struct MakeWindow : public Window
{
    MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
        : Window(hWnd, windowName, clientWidth, clientHeight, vSync)
    {}
};


void DxEngine::Create(HINSTANCE hInst)
{
}

DxEngine& DxEngine::Get()
{
}

void DxEngine::Destroy()
{
}

std::shared_ptr<Window> DxEngine::CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight,
	bool vSync)
{
}

void DxEngine::DestroyWindow(std::shared_ptr<Window> window)
{
}

std::shared_ptr<Window> DxEngine::GetWindowByName(const std::wstring& windowName)
{
}

int DxEngine::Run(std::shared_ptr<IDemo> pGame)
{
}

void DxEngine::Quit(int exitCode)
{
}

void DxEngine::Flush()
{
}

ComPtr<ID3D12DescriptorHeap> DxEngine::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
}

bool DxEngine::IsTearingSupported() const
{
}

DXGI_SAMPLE_DESC DxEngine::GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples,
	D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags) const
{
}

ComPtr<ID3D12Device> DxEngine::GetDevice() const
{
}

ComPtr<IDXGIFactory7> DxEngine::GetFactory() const
{
}

ComPtr<IDXGISwapChain4> DxEngine::GetSwapChain() const
{
}

std::shared_ptr<CommandQueue> DxEngine::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
}

UINT DxEngine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
}

std::pair<int, int> DxEngine::GetScreenDimension() const
{
}

DxEngine::DxEngine(HINSTANCE hInst)
{
}

void DxEngine::Initialize()
{
}

ComPtr<IDXGIAdapter4> DxEngine::GetAdapter(bool bUseWarp)
{
}

ComPtr<ID3D12Device2> DxEngine::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
}

bool DxEngine::CheckTearingSupport()
{
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
}
