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
{
}

bool IKDemo::LoadContent()
{
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", *cmdList);
	mModels["IkObject"] = std::make_shared<Model>("../models/IkObject.dae", *cmdList);

	mShaders["ForwardVS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ForwardPS"] = DxUtil::CompileShader(L"../shaders/GeometryPass.hlsl", nullptr, "PS", "ps_5_1");

	mObjects.push_back(std::make_shared<Object>(mModels["Plane"], XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1)));

	mForwardPass = std::make_unique<ForwardPass>(mShaders["ForwardVS"], mShaders["ForwardPS"]);

	InitDescriptorHeaps();
	InitRenderTarget();

	return true;
}

void IKDemo::UnloadContent()
{

}

void IKDemo::OnUpdate(UpdateEventArgs& e)
{
	IDemo::OnUpdate(e);
}

void IKDemo::OnRender(RenderEventArgs& e)
{
	IDemo::OnRender(e);
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	auto rtvCpuHandle = mDescriptorHeaps[HeapType::RTV]->GetCpuHandle(mRtvIdx);
	auto dsvCpuHandle = mDescriptorHeaps[HeapType::DSV]->GetCpuHandle(mDsvIdx);

	float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	//TODO
	cmdList->ClearTexture(mRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
	cmdList->ClearDepthStencilTexture(mRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH| D3D12_CLEAR_FLAG_STENCIL);
	cmdList->SetPipelineState(mForwardPass->mPSO);
	cmdList->SetGraphicsRootSignature(mForwardPass->mRootSig);

	/*cmdList->SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());
	cmdList->SetViewport(mScreenViewport);
	cmdList->SetScissorRect(mScissorRect);
	cmdList->SetRenderTargets(rtvArray, &dsvHeapCPUHandle);*/
	for (const auto& object : mObjects)
	{
		object->Draw(*cmdList);
	}
	commandQueue->ExecuteCommandList(cmdList);
	m_pWindow->Present();
}

void IKDemo::InitRenderTarget()
{
	DXGI_SAMPLE_DESC sampleDesc = DxEngine::Get().GetMultisampleQualityLevels(BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN), D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT);
	auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(BufferFormat::GetBufferFormat(BufferType::SWAPCHAIN), mWidth, mHeight,
		1, 1,
		sampleDesc.Count, sampleDesc.Quality,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	D3D12_CLEAR_VALUE colorClearValue;
	colorClearValue.Format = colorDesc.Format;
	colorClearValue.Color[0] = 0.4f;
	colorClearValue.Color[1] = 0.6f;
	colorClearValue.Color[2] = 0.9f;
	colorClearValue.Color[3] = 1.0f;

	Texture colorTexture = Texture(colorDesc, &colorClearValue,
		TextureUsage::RenderTarget,
		L"Color Render Target");

	// Create a depth buffer.
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(BufferFormat::GetBufferFormat(BufferType::DEPTH_STENCIL_DSV),
		mWidth, mHeight,
		1, 1,
		sampleDesc.Count, sampleDesc.Quality,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	Texture depthTexture = Texture(depthDesc, &depthClearValue,
		TextureUsage::Depth,
		L"Depth Render Target");

	CD3DX12_RESOURCE_DESC rtvDesc(colorTexture.GetD3D12ResourceDesc());
	mRtvIdx = mDescriptorHeaps[HeapType::RTV]->GetNextAvailableIndex();
	DxEngine::Get().CreateRtvDescriptor(rtvDesc.Format, colorTexture.GetD3D12Resource(), mDescriptorHeaps[RTV]->GetCpuHandle(mRtvIdx));

	CD3DX12_RESOURCE_DESC dsvDesc(depthTexture.GetD3D12ResourceDesc());
	mDsvIdx = mDescriptorHeaps[HeapType::DSV]->GetNextAvailableIndex();
	DxEngine::Get().CreateDsvDescriptor(dsvDesc.Format, depthTexture.GetD3D12Resource(), mDescriptorHeaps[DSV]->GetCpuHandle(mDsvIdx));

	mRenderTarget.AttachTexture(AttachmentPoint::Color0, colorTexture);
	mRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
}

void IKDemo::InitDescriptorHeaps()
{
	mDescriptorHeaps[HeapType::RTV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), 128);
	mDescriptorHeaps[HeapType::DSV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), 128);
}

void IKDemo::UpdateConstantBuffer()
{

}