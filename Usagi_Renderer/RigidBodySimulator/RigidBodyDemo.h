#pragma once
#include "IDemo.h"
#include "Camera.h"
#include "StructuredBuffer.h"
#include "d3dx12.h"
#include "IndexBuffer.h"

#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
struct Particle 
{
	XMVECTOR pos;
	XMVECTOR vel;
	XMVECTOR normal;
	float pinned;
	XMFLOAT3 _pad0;
};

class CommandList;
class ComputePass;
class RigidBodyDemo : public IDemo
{
public:
	RigidBodyDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent();
	void UnloadContent();

private:
	void PrepareBuffers(CommandList& cmdList);
	void Draw(CommandList& cmdList);
private:
	Camera mCamera;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	POINT mLastMousePos;

	ComPtr<ID3D12DescriptorHeap> mGuiSrvUavCbvHeap = NULL;
	
	int mWidth = 0;
	int mHeight = 0;

private:
	ComPtr<ID3D12DescriptorHeap> mDemoSrvUavCbvHeap = NULL;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mUavInputDesc;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mUavOutputDesc;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	IndexBuffer mIndexBuffer;

	std::unique_ptr<StructuredBuffer> mVertexInput;
	std::unique_ptr<StructuredBuffer> mVertexOutput;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unique_ptr<ComputePass> mComputePass;

private:
	struct Cloth {
		XMFLOAT2 gridsize = XMFLOAT2(60, 60);
		XMFLOAT2 size = XMFLOAT2(5.0f, 5.0f);
	} cloth;
};

