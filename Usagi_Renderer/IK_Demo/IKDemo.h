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
#include <IMGUI/imgui.h>
#include <DirectXMath.h>

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
using namespace DirectX;

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
    void InitGui();

    void UpdateConstantBuffer(UpdateEventArgs& e);
    void UpdateTargetPos();
    void UpdateIkObject();
    void UpdateHirarchyTest();
    
    void DrawObject(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
    void DrawLine(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle);
    void DrawIkBone(CommandList& cmdList);

    void StartGuiFrame();
    void UpdateGui();
    void ClearGui();
    void DrawGui(CommandList& cmdList);

    XMVECTOR GetDecomposedTranslation(XMMATRIX matrix);
    float GetDistance(XMVECTOR lhs, XMVECTOR rhs);
    XMMATRIX GetInverseMatrix(XMMATRIX mat);

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

    std::vector<XMMATRIX> mJointMats;//0'th joint is Root, (n-1)'th joint is EE. Every joint define relatively.
    int mJointNum;
    int mEEIdx;
    XMMATRIX mIkPosition;

    bool breakButton = false;

    std::shared_ptr<Object> mTarget;
    XMFLOAT4 mTargetPosition = XMFLOAT4(0.4, 0.4, 0.0, 1);

    std::unique_ptr<ForwardPass> mForwardPass;
    std::unique_ptr<LinePass> mLinePass;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	Camera mCamera;

    POINT mLastMousePos;

    ComPtr<ID3D12DescriptorHeap> mGuiSrvUavCbvHeap = NULL;

	int mWidth = 0;
	int mHeight = 0;
};