#include "pch.h"
#include "Window.h"
#include "DxEngine.h"

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
}

Window::~Window()
{

}


HWND Window::GetWindowHandle() const
{
}

void Window::Initialize()
{
}

void Window::Destroy()
{
}

const std::wstring& Window::GetWindowName() const
{
}

int Window::GetClientWidth() const
{
}

int Window::GetClientHeight() const
{
}

bool Window::IsVSync() const
{
}

void Window::SetVSync(bool vSync)
{
}

void Window::ToggleVSync()
{
}

bool Window::IsFullScreen() const
{
}

void Window::SetFullscreen(bool fullscreen)
{
}

void Window::ToggleFullscreen()
{
}

void Window::Show()
{
}

void Window::Hide()
{
}


void Window::RegisterDemo(std::shared_ptr<IDemo> pGame)
{
}

void Window::OnUpdate(UpdateEventArgs& e)
{
}

void Window::OnRender(RenderEventArgs& e)
{
}

void Window::OnKeyPressed(KeyEventArgs& e)
{
}

void Window::OnKeyReleased(KeyEventArgs& e)
{
}

void Window::OnMouseMoved(MouseMotionEventArgs& e)
{
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
}

void Window::OnMouseWheel(MouseWheelEventArgs& e)
{
}

void Window::OnResize(ResizeEventArgs& e)
{
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> Window::CreateSwapChain()
{
}

void Window::UpdateRenderTargetViews()
{
}
