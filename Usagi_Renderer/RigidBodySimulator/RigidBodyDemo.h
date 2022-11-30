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

#include "RenderTarget.h"

class Model;
class Object;
using Microsoft::WRL::ComPtr;
using namespace DirectX;
struct CommonCB
{
	XMFLOAT4X4 View = MathHelper::Identity4x4();
	XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	XMFLOAT4X4 ViewProjTex = MathHelper::Identity4x4();
	XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;
};

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
	XMFLOAT4 gravity = XMFLOAT4(0.0f, -9.8f, 0.0f, 0.0f);
	XMFLOAT2 particleCount;
	XMFLOAT2 padding_0;
};

class CommandList;
class ComputePass;
class DrawPass;
class ForwardPass;
class RigidBodyDemo : public IDemo
{
public:
	RigidBodyDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent();
	void UnloadContent();

	void OnUpdate(UpdateEventArgs& e);
	void OnRender(RenderEventArgs& e);

private:
	void InitDescriptorHeaps();
	void InitRenderTarget();
	void PrepareBuffers(CommandList& cmdList);

	void ComputeCall(CommandList& cmdList);
	void ClothRenderCall(CommandList& cmdList);
	void ObjectRenderCall(CommandList& cmdList);

	void UpdateConstantBuffer(UpdateEventArgs& e);

	void OnMouseMoved(MouseMotionEventArgs& e);
	void OnMouseButtonPressed(MouseButtonEventArgs& e);
	void OnMouseButtonReleased(MouseButtonEventArgs& e);

	void InitGui();
	void StartGuiFrame();
	void UpdateGui();
	void ClearGui();
	void DrawGui(CommandList& cmdList);
private:
	Camera mCamera;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	POINT mLastMousePos;

	ComPtr<ID3D12DescriptorHeap> mGuiSrvUavCbvHeap = NULL;

	std::map<HeapType, std::shared_ptr<DescriptorHeap>> mDescriptorHeaps;

	RenderTarget mRenderTarget;
	UINT mRtvIdx;
	UINT mDsvIdx;

	int mWidth = 0;
	int mHeight = 0;

	std::shared_ptr<Object> mSphere;
	std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
	CommonCB mCommonCB;

private:
	CD3DX12_CPU_DESCRIPTOR_HANDLE mUavInputDesc;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mUavOutputDesc;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	IndexBuffer mIndexBuffer;
	UINT mClothIndexNum;

	std::unique_ptr<StructuredBuffer> mVertexInput;
	std::unique_ptr<StructuredBuffer> mVertexOutput;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unique_ptr<ComputePass> mComputePass;
	std::unique_ptr<DrawPass> mDrawPass;
	std::unique_ptr<ForwardPass> mForwardPass;

	ComputeConstants computeDatas;

	XMFLOAT4X4 mViewProjection;

	float mRemovePin = 0.f;

private:
	struct Cloth {
		XMFLOAT2 gridsize = XMFLOAT2(60, 60);
		XMFLOAT2 size = XMFLOAT2(5.0f, 5.0f);
	} cloth;
};

