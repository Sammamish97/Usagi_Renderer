#include "pch.h"
#include "IDemo.h"
#include "DxEngine.h"
#include "Window.h"

IDemo::IDemo(const std::wstring& name, int width, int height, bool vSync)
    : mName(name)
    , mWidth(width)
    , mHeight(height)
    , mvSync(vSync)
{
}

IDemo::~IDemo()
{
}

bool IDemo::Initialize()
{
}

bool IDemo::LoadContent()
{
}

void IDemo::UnloadContent()
{
}

void IDemo::Destroy()
{
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
