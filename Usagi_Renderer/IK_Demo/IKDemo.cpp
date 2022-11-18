#include "IKDemo.h"
#include "DxEngine.h"
#include "DxUtil.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "Object.h"
#include "Window.h"
#include "BufferFormat.h"
#include "ForwardPass.h"
#include "Model.h"

IKDemo::IKDemo(const std::wstring& name, int width, int height, bool vSync)
	:IDemo(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, mWidth(width)
	, mHeight(height)
	, mCamera(width / static_cast<float>(height))
{
}

bool IKDemo::LoadContent()
{
	InitDescriptorHeaps();

	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();
	
	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", *cmdList);
	//mModels["IkObject"] = std::make_shared<Model>("../models/IKBone.dae", *cmdList);
	mModels["Monkey"] = std::make_shared<Model>("../models/Monkey.obj", *cmdList);

	mShaders["ForwardVS"] = DxUtil::CompileShader(L"../shaders/Forward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ForwardPS"] = DxUtil::CompileShader(L"../shaders/Forward.hlsl", nullptr, "PS", "ps_5_1");

	//mObjects.push_back(std::make_shared<Object>(mModels["Plane"], XMFLOAT3(0, -1, 0), XMFLOAT3(1, 1, 1)));
	mObjects.push_back(std::make_shared<Object>(mModels["IkObject"], XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1)));

	mForwardPass = std::make_unique<ForwardPass>(mShaders["ForwardVS"], mShaders["ForwardPS"]);

	InitDescriptorHeaps();
	InitRenderTarget();

	auto fenceValue = commandQueue->ExecuteCommandList(cmdList);
	commandQueue->WaitForFenceValue(fenceValue);
	return true;
}

void IKDemo::UnloadContent()
{

}

void IKDemo::OnUpdate(UpdateEventArgs& e)
{
	IDemo::OnUpdate(e);
	mCamera.Update(e);
	UpdateConstantBuffer(e);
}

void IKDemo::OnRender(RenderEventArgs& e)
{
	IDemo::OnRender(e);
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	auto rtvCpuHandle = mDescriptorHeaps[HeapType::RTV]->GetCpuHandle(mRtvIdx);
	auto dsvCpuHandle = mDescriptorHeaps[HeapType::DSV]->GetCpuHandle(mDsvIdx);

	float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	cmdList->ClearTexture(mRenderTarget.GetTexture(AttachmentPoint::Color0), rtvCpuHandle, clearColor);
	cmdList->ClearDepthStencilTexture(mRenderTarget.GetTexture(AttachmentPoint::DepthStencil), dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH| D3D12_CLEAR_FLAG_STENCIL);
	cmdList->SetPipelineState(mForwardPass->mPSO);
	cmdList->SetGraphicsRootSignature(mForwardPass->mRootSig);

	cmdList->SetViewport(m_Viewport);
	cmdList->SetScissorRect(m_ScissorRect);

	cmdList->SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), &mCommonCB);
	cmdList->SetGraphicsDynamicConstantBuffer(2, sizeof(DirectLight), &mLightCB);

	cmdList->SetSingleRenderTarget(&rtvCpuHandle, &dsvCpuHandle);

	for (const auto& object : mObjects)
	{
		object->Draw(*cmdList);
	}
	commandQueue->ExecuteCommandList(cmdList);
	m_pWindow->Present(mRenderTarget.GetTexture(AttachmentPoint::Color0));
}

void IKDemo::InitRenderTarget()
{
	//DXGI_SAMPLE_DESC sampleDesc = DxEngine::Get().GetMultisampleQualityLevels(BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN), D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN), mWidth, mHeight,
		1, 1,
		1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	Texture colorTexture = Texture(colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"Color Render Target");

	// Create a depth buffer.
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(BufferFormat::GetBufferFormat(BufferType::DEPTH_STENCIL_DSV),
		mWidth, mHeight,
		1, 1,
		1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	Texture depthTexture = Texture(depthDesc, &depthClearValue,
		TextureUsage::Depth,
		L"Depth Render Target");

	CD3DX12_RESOURCE_DESC rtvDesc(colorTexture.GetD3D12ResourceDesc());
	mRtvIdx = mDescriptorHeaps[HeapType::RTV]->GetNextAvailableIndex();
	DxEngine::Get().CreateRtvDescriptor(rtvDesc.Format, colorTexture.GetD3D12Resource(), mDescriptorHeaps[HeapType::RTV]->GetCpuHandle(mRtvIdx));

	CD3DX12_RESOURCE_DESC dsvDesc(depthTexture.GetD3D12ResourceDesc());
	mDsvIdx = mDescriptorHeaps[HeapType::DSV]->GetNextAvailableIndex();
	DxEngine::Get().CreateDsvDescriptor(dsvDesc.Format, depthTexture.GetD3D12Resource(), mDescriptorHeaps[HeapType::DSV]->GetCpuHandle(mDsvIdx));

	mRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	mRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
}

void IKDemo::InitDescriptorHeaps()
{
	mDescriptorHeaps[HeapType::RTV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), 128);
	mDescriptorHeaps[HeapType::DSV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), 128);
}

void IKDemo::UpdateConstantBuffer(UpdateEventArgs& e)
{
	XMMATRIX view = XMLoadFloat4x4(&mCamera.GetViewMat());
	XMMATRIX proj = XMLoadFloat4x4(&mCamera.GetProjMat());

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mCommonCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mCommonCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mCommonCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mCommonCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mCommonCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mCommonCB.InvViewProj, XMMatrixTranspose(invViewProj));

	mCommonCB.EyePosW = mCamera.GetPosition();
	mCommonCB.RenderTargetSize = XMFLOAT2((float)mWidth, (float)mHeight);
	mCommonCB.InvRenderTargetSize = XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
	mCommonCB.NearZ = 1.0f;
	mCommonCB.FarZ = 1000.0f;

	mCommonCB.TotalTime = e.TotalTime; 
	mCommonCB.DeltaTime = e.ElapsedTime;

	mLightCB.Direction = XMFLOAT3(-0.5f, 0.5f, 0.5f);
	mLightCB.Color = XMFLOAT3(1, 1, 1);
}

void IKDemo::OnMouseMoved(MouseMotionEventArgs& e)
{
	int x = e.X;
	int y = e.Y;

	if (e.LeftButton != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mCamera.mTheta += dx;
		mCamera.mPhi += dy;

		// Restrict the angle mPhi.
		mCamera.mPhi = MathHelper::Clamp(mCamera.mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if (e.RightButton != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mCamera.mRadius += dx - dy;

		// Restrict the radius.
		mCamera.mRadius = MathHelper::Clamp(mCamera.mRadius, 5.0f, 150.0f);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
void IKDemo::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
	mLastMousePos.x = e.X;
	mLastMousePos.y = e.Y;
}
void IKDemo::OnMouseButtonReleased(MouseButtonEventArgs& e)
{

}