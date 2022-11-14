#pragma once
#include "Events.h"
#include <memory>
#include <string>

class Window;

class IDemo : public std::enable_shared_from_this<IDemo>
{
public:
    IDemo(const std::wstring& name, int width, int height, bool vSync);
    virtual ~IDemo();

    int GetClientWidth() const
    {
        return mWidth;
    }

    int GetClientHeight() const
    {
        return mHeight;
    }

    virtual bool Initialize();
    virtual bool LoadContent() = 0;
    virtual void UnloadContent() = 0;
    virtual void Destroy();

protected:
    friend class Window;
    virtual void OnUpdate(UpdateEventArgs& e);
    virtual void OnRender(RenderEventArgs& e);
    virtual void OnKeyPressed(KeyEventArgs& e);
    virtual void OnKeyReleased(KeyEventArgs& e);
    virtual void OnMouseMoved(MouseMotionEventArgs& e);
    virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
    virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
    virtual void OnMouseWheel(MouseWheelEventArgs& e);
    virtual void OnResize(ResizeEventArgs& e);
    virtual void OnWindowDestroy();

    std::shared_ptr<Window> m_pWindow;

private:
    std::wstring mName;
    int mWidth;
    int mHeight;
    bool mvSync;
};

