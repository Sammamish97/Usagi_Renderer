#pragma once
#include "IDemo.h"
#include "Camera.h"
#include "StructuredBuffer.h"

#include <d3d12.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class RigidBodyDemo : public IDemo
{
public:
	RigidBodyDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent();
	void UnloadContent();

private:
	Camera mCamera;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	POINT mLastMousePos;

	ComPtr<ID3D12DescriptorHeap> mGuiSrvUavCbvHeap = NULL;

	int mWidth = 0;
	int mHeight = 0;

private:
	StructuredBuffer mParticleIn;
	StructuredBuffer mParticleOut;
};

