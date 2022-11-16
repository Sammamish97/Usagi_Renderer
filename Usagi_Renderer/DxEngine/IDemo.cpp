#include "pch.h"
#include "IDemo.h"
#include "DxEngine.h"
#include "Window.h"
#include <DirectXMath.h>
#include <cassert>

IDemo::IDemo(const std::wstring& name, int width, int height, bool vSync)
    : mName(name)
    , mWidth(width)
    , mHeight(height)
    , mvSync(vSync)
{
}

IDemo::~IDemo()
{
    assert(!m_pWindow && "Use Game::Destroy() before destruction.");
}

bool IDemo::Initialize()
{
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    m_pWindow = DxEngine::Get().CreateRenderWindow(mName, mWidth, mHeight, mvSync);
    m_pWindow->RegisterDemo(shared_from_this());
    m_pWindow->Show();

    return true;
}

void IDemo::Destroy()
{
    DxEngine::Get().DestroyWindow(m_pWindow);
    m_pWindow.reset();
}

void IDemo::OnUpdate(UpdateEventArgs& e)
{
}

void IDemo::OnRender(RenderEventArgs& e)
{
}

void IDemo::OnKeyPressed(KeyEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnKeyReleased(KeyEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnMouseMoved(MouseMotionEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnMouseWheel(MouseWheelEventArgs& e)
{
    // By default, do nothing.
}

void IDemo::OnResize(ResizeEventArgs& e)
{
}

void IDemo::OnWindowDestroy()
{
    UnloadContent();
}
