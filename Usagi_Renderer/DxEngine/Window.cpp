#include "pch.h"
#include "Window.h"
#include "DxEngine.h"
#include "IDemo.h"
#include "DxUtil.h"
#include "BufferFormat.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ResourceStateTracker.h"
#include <cassert>

Window::Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
    : mHwnd(hWnd)
    , mWindowName(windowName)
    , mWidth(clientWidth)
    , mHeight(clientHeight)
    , mVSync(vSync)
    , mFullScreen(false)
    , mFenceValues{ 0 }
    , mFrameValues{ 0 }
{
    DxEngine& engine = DxEngine::Get();
    mIsTearingSupported = engine.IsTearingSupported();

    for (int i = 0; i < BufferCount; ++i)
    {
        mBackBufferTextures[i].SetName(L"Backbuffer[" + std::to_wstring(i) + L"]");
    }
    mdxgiSwapChain = CreateSwapChain();
    UpdateRenderTargetViews();
}

Window::~Window()
{
    assert(!mHwnd && "Use Application::DestroyWindow before destruction.");
}

void Window::Initialize()
{
}

HWND Window::GetWindowHandle() const
{
    return mHwnd;
}

void Window::Destroy()
{
    if (auto pDemo = mDemo.lock())
    {
        // Notify the registered game that the window is being destroyed.
        pDemo->OnWindowDestroy();
    }

    if (mHwnd)
    {
        DestroyWindow(mHwnd);
        mHwnd = nullptr;
    }
}

const std::wstring& Window::GetWindowName() const
{
    return mWindowName;
}

int Window::GetClientWidth() const
{
    return mWidth;
}

int Window::GetClientHeight() const
{
    return mHeight;
}

bool Window::IsVSync() const
{
    return mVSync;
}

void Window::SetVSync(bool vSync)
{
    mVSync = vSync;
}

void Window::ToggleVSync()
{
    SetVSync(!mVSync);
}

void Window::Show()
{
    ::ShowWindow(mHwnd, SW_SHOW);
}

void Window::Hide()
{
    ::ShowWindow(mHwnd, SW_HIDE);
}

void Window::RegisterDemo(std::shared_ptr<IDemo> pGame)
{
    mDemo = pGame;
    return;
}

void Window::OnUpdate(UpdateEventArgs& e)
{
    m_UpdateClock.Tick();
    if (auto pDemo = mDemo.lock())
    {
        UpdateEventArgs updateEventArgs(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds(), e.FrameNumber);
        pDemo->OnUpdate(updateEventArgs);
    }
}

void Window::OnRender(RenderEventArgs& e)
{
    m_RenderClock.Tick();

    if (auto pDemo = mDemo.lock())
    {
        RenderEventArgs renderEventArgs(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds(), e.FrameNumber);
        pDemo->OnRender(renderEventArgs);
    }
}

void Window::OnKeyPressed(KeyEventArgs& e)
{
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnKeyPressed(e);
    }
}

void Window::OnKeyReleased(KeyEventArgs& e)
{
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnKeyReleased(e);
    }
}

void Window::OnMouseMoved(MouseMotionEventArgs& e)
{
    e.RelX = e.X - mPreviousMouseX;
    e.RelY = e.Y - mPreviousMouseY;

    mPreviousMouseX = e.X;
    mPreviousMouseY = e.Y;
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnMouseMoved(e);
    }
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    mPreviousMouseX = e.X;
    mPreviousMouseY = e.Y;
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnMouseButtonPressed(e);
    }
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnMouseButtonReleased(e);
    }
}

void Window::OnMouseWheel(MouseWheelEventArgs& e)
{
    if (auto pDemo = mDemo.lock())
    {
        pDemo->OnMouseWheel(e);
    }
}

void Window::OnResize(ResizeEventArgs& e)
{
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
    DxEngine& engine = DxEngine::Get();

    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)))

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = mWidth;
    swapChainDesc.Height = mHeight;
    swapChainDesc.Format = BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN);
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = BufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = mIsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ID3D12CommandQueue* pCommandQueue = engine.GetCommandQueue()->GetD3D12CommandQueue().Get();

    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        pCommandQueue,
        mHwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1))
    //TODO: Need to understand.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER))
    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));
    mCurrentBackBufferIndex = dxgiSwapChain4->GetCurrentBackBufferIndex();
    return dxgiSwapChain4;
}

void Window::UpdateRenderTargetViews()
{
    for (int i = 0; i < BufferCount; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(mdxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_COMMON);

        mBackBufferTextures[i].SetD3D12Resource(backBuffer);
    }
}

UINT Window::Present(const Texture& texture)
{
    auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    auto& backBuffer = mBackBufferTextures[mCurrentBackBufferIndex];

    if (texture.IsValid())
    {
        if (texture.GetD3D12ResourceDesc().SampleDesc.Count > 1)
        {
            commandList->ResolveSubresource(backBuffer, texture);
        }
        else
        {
            commandList->CopyResource(backBuffer, texture);
        }
    }

    RenderTarget renderTarget;
    renderTarget.AttachTexture(AttachmentPoint::Color0, backBuffer);

    //m_GUI.Render(commandList, renderTarget);

    commandList->TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT);
    commandQueue->ExecuteCommandList(commandList);

    UINT syncInterval = mVSync ? 1 : 0;
    UINT presentFlags = mIsTearingSupported && !mVSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(mdxgiSwapChain->Present(syncInterval, presentFlags));

    mFenceValues[mCurrentBackBufferIndex] = commandQueue->Signal();
    mFrameValues[mCurrentBackBufferIndex] = DxEngine::GetFrameCount();

    mCurrentBackBufferIndex = mdxgiSwapChain->GetCurrentBackBufferIndex();

    commandQueue->WaitForFenceValue(mFenceValues[mCurrentBackBufferIndex]);

    return mCurrentBackBufferIndex;
}