#pragma once
#define NOMINMAX
#include "IDemo.h"
#include "RenderTarget.h"
#include "Texture.h"
#include "Camera.h"
#include "DescriptorHeap.h"
#include "ForwardPass.h"
#include "LinePass.h"
#include <map>
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
class Model;
class BoneModel;
class CommandList;
class ForwardPass;
class LinePass;
class IKDemo : public IDemo
{
public:
	IKDemo(const std::wstring& name, int width, int height, bool vSync = false);
	bool LoadContent() override;
	void UnloadContent() override;
	void OnUpdate(UpdateEventArgs& e);
	void OnRender(RenderEventArgs& e);
    void OnMouseMoved(MouseMotionEventArgs& e);
    void OnMouseButtonPressed(MouseButtonEventArgs& e);
    void OnMouseButtonReleased(MouseButtonEventArgs& e);

private:
    void InitBoneObject(int linkNum = 5);
    void InitDescriptorHeaps();
    void InitRenderTarget();

    void UpdateConstantBuffer(UpdateEventArgs& e);
    
    void DrawObject(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
    void DrawLine(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
    void DrawIkBone(CommandList& cmdList);

private:
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12PipelineState> mPipelineState;

    std::map<HeapType, std::shared_ptr<DescriptorHeap>> mDescriptorHeaps;

    RenderTarget mRenderTarget;
    UINT mRtvIdx;
    UINT mDsvIdx;

    CommonCB mCommonCB;
    int mCommonIdx;
    DirectLight mLightCB;
    int mLightIdx;
    
    std::vector<std::shared_ptr<Object>> mObjects;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, std::shared_ptr<Model>> mModels;
    std::shared_ptr<BoneModel> mIkModel;
    int mJointNum;
    int mEEIdx;

    std::unique_ptr<ForwardPass> mForwardPass;
    std::unique_ptr<LinePass> mLinePass;

    std::vector<aiMatrix4x4> mBoneMats;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	Camera mCamera;

    POINT mLastMousePos;

	int mWidth = 0;
	int mHeight = 0;
};

