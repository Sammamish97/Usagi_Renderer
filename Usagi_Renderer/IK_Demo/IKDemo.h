#pragma once
#include "IDemo.h"
#include "RenderTarget.h"
#include "Texture.h"
#include "Camera.h"
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

struct DirectLight
{
    XMFLOAT3 Direction;
    float padding1;

    XMFLOAT3 Color;
    float padding2;
};

using Microsoft::WRL::ComPtr;

class Object;
class IKDemo : public IDemo
{
public:
	IKDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent() override;
	void UnloadContent() override;
	void OnUpdate(UpdateEventArgs& e);
	void OnRender(RenderEventArgs& e);

private:
    void InitPSO();
    void InitRootSignature();
    void InitDescriptors();

private:
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPipelineState;

    ComPtr<ID3D12DescriptorHeap> mRtvDescHeap;
    ComPtr<ID3D12DescriptorHeap> mDsvDescHeap;
    ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

    RenderTarget mRenderTarget;

    CommonCB mCommonCB;
    int mCommonIdx;
    DirectLight mLightCB;
    int mLightIdx;
    
    std::vector<Object> mObjects;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	Camera mCamera;

	int mWidth;
	int mHeight;
};

