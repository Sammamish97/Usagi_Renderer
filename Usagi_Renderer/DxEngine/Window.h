#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <memory>
#include <string>

#include "Events.h"
#include "HighResolutionClock.h"
#include "RenderTarget.h"
#include "Texture.h"

class IDemo;

class Window : public std::enable_shared_from_this<Window>
{
public:
	static const UINT BufferCount = 3;

	HWND GetWindowHandle() const;

	void Initialize();

	void Destroy();

	const std::wstring& GetWindowName() const;

	int GetClientWidth() const;
	int GetClientHeight() const;

	bool IsVSync() const;
	void SetVSync(bool vSync);
	void ToggleVSync();

	void Show();
	void Hide();
	const RenderTarget& GetRenderTarget() const;
	UINT Present(const Texture& texture = Texture());

protected:
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	friend class IDemo;
	friend class DxEngine;
	Window() = delete;
	Window(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync);
	virtual ~Window();

	void RegisterDemo(std::shared_ptr<IDemo> pGame);

	// Update and Draw can only be called by the application.
	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnRender(RenderEventArgs& e);

	// A keyboard key was pressed
	virtual void OnKeyPressed(KeyEventArgs& e);
	// A keyboard key was released
	virtual void OnKeyReleased(KeyEventArgs& e);

	// The mouse was moved
	virtual void OnMouseMoved(MouseMotionEventArgs& e);
	// A button on the mouse was pressed
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
	// A button on the mouse was released
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
	// The mouse wheel was moved.
	virtual void OnMouseWheel(MouseWheelEventArgs& e);

	// The window was resized.
	virtual void OnResize(ResizeEventArgs& e);

	// Create the swapchian.
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

	// Update the render target views for the swapchain back buffers.
	void UpdateRenderTargetViews();
private:
	Window(const Window& copy) = delete;
	Window& operator=(const Window& other) = delete;

	HWND mHwnd;
	std::wstring mWindowName;

	int mWidth;
	int mHeight;
	bool mVSync;
	bool mFullScreen;

	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;

	UINT64 mFenceValues[BufferCount];
	uint64_t mFrameValues[BufferCount];

	std::weak_ptr<IDemo> mDemo;

	ComPtr<IDXGISwapChain4> mdxgiSwapChain;
	Texture mBackBufferTextures[BufferCount];
	// Marked mutable to allow modification in a const function.
	mutable RenderTarget mRenderTarget;

	UINT mCurrentBackBufferIndex;

	RECT mWindowRect;
	bool mIsTearingSupported;

	int mPreviousMouseX;
	int mPreviousMouseY;
};

