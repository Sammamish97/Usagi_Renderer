#pragma once
#include "IDemo.h"
#include "Camera.h"
#include "StructuredBuffer.h"
#include "d3dx12.h"
#include "IndexBuffer.h"
#include "DescriptorHeap.h"

#include <d3d12.h>
#include <wrl.h>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include <array>

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

struct ComputeConstants
{
	float deltaT = 0.0f;
	float particleMass = 0.1f;
	float springStiffness = 2000.0f;
	float damping = 0.25f;
	float restDistH;
	float restDistV;
	float restDistD;
	float sphereRadius = 1.0f;
	XMFLOAT4 spherePos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	XMFLOAT4 gravity = XMFLOAT4(0.0f, 9.8f, 0.0f, 0.0f);
	XMFLOAT2 particleCount;
};

class CommandList;
class ComputePass;
class DrawPass;
class RigidBodyDemo : public IDemo
{
public:
	RigidBodyDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent();
	void UnloadContent();

	void OnUpdate(UpdateEventArgs& e);
	void OnRender(RenderEventArgs& e);

private:
	void PrepareBuffers(CommandList& cmdList);
	void Draw(CommandList& cmdList);

	void BuildComputeList();
	void BuildGraphicsList();
	void BuildComputeDescriptorHeaps();

private:
	Camera mCamera;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	POINT mLastMousePos;

	ComPtr<ID3D12DescriptorHeap> mGuiSrvUavCbvHeap = NULL;
	
	int mWidth = 0;
	int mHeight = 0;

private:
	std::array<std::unique_ptr<DescriptorHeap>, 2> mComputeDescHeaps;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mUavInputDesc;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mUavOutputDesc;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	IndexBuffer mIndexBuffer;

	std::unique_ptr<StructuredBuffer> mVertexInput;
	std::unique_ptr<StructuredBuffer> mVertexOutput;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unique_ptr<ComputePass> mComputePass;
	std::unique_ptr<DrawPass> mDrawPass;

	ComputeConstants computeDatas;
private:
	struct Cloth {
		XMFLOAT2 gridsize = XMFLOAT2(60, 60);
		XMFLOAT2 size = XMFLOAT2(5.0f, 5.0f);
	} cloth;
};

