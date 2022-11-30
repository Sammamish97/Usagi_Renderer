#include "RigidBodyDemo.h"

#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_dx12.h>
#include <IMGUI/imgui_impl_win32.h>

#include "BufferFormat.h"
#include "d3dx12.h"
#include "DxEngine.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ComputePass.h"
#include "DrawPass.h"
#include "DxUtil.h"
#include "ResourceStateTracker.h"
#include "Window.h"
#include "Model.h"
#include "Object.h"
#include "ForwardPass.h"

RigidBodyDemo::RigidBodyDemo(const std::wstring& name, int width, int height, bool vSync)
	:IDemo(name, width, height, vSync)
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, mWidth(width)
	, mHeight(height)
	, mCamera(width / static_cast<float>(height))
{
	
}

bool RigidBodyDemo::LoadContent()
{
	InitGui();

	auto device = DxEngine::Get().GetDevice();
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	PrepareBuffers(*cmdList);
	InitDescriptorHeaps();
	InitRenderTarget();

	mModels["Sphere"] = std::make_shared<Model>("../models/Sphere.obj", *cmdList);
	mSphere = std::make_shared<Object>(mModels["Sphere"], XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(1, 1, 1));

	mShaders["RigidCS"] = DxUtil::CompileShader(L"../shaders/RigidCompute.hlsl", nullptr, "CS", "cs_5_1");

	mShaders["SimpleForwardVS"] = DxUtil::CompileShader(L"../shaders/SimpleForward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SimpleForwardPS"] = DxUtil::CompileShader(L"../shaders/SimpleForward.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["RigidForwardVS"] = DxUtil::CompileShader(L"../shaders/RigidForward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["RigidForwardPS"] = DxUtil::CompileShader(L"../shaders/RigidForward.hlsl", nullptr, "PS", "ps_5_1");

	mComputePass = std::make_unique<ComputePass>(mShaders["RigidCS"]);
	mDrawPass = std::make_unique<DrawPass>(mShaders["RigidForwardVS"], mShaders["RigidForwardPS"]);
	mForwardPass = std::make_unique<ForwardPass>(mShaders["SimpleForwardVS"], mShaders["SimpleForwardPS"]);

	auto fenceValue = commandQueue->ExecuteCommandList(cmdList);
	commandQueue->WaitForFenceValue(fenceValue);
	return true;
}

void RigidBodyDemo::UnloadContent()
{
	ClearGui();
}

void RigidBodyDemo::InitDescriptorHeaps()
{
	mDescriptorHeaps[HeapType::RTV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), 128);
	mDescriptorHeaps[HeapType::DSV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), 128);
}

void RigidBodyDemo::InitRenderTarget()
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

void RigidBodyDemo::PrepareBuffers(CommandList& cmdList)
{
	D3D12_RESOURCE_FLAGS srvFlags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_FLAGS uavFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	auto totalGrids = cloth.gridsize.x * cloth.gridsize.y;
	auto particleSize = sizeof(Particle);
	auto totalSize = totalGrids * particleSize;

	mVertexInput = std::make_unique<StructuredBuffer>(CD3DX12_RESOURCE_DESC::Buffer(totalSize, srvFlags), totalGrids, particleSize, L"Particle_Input");
	mVertexOutput = std::make_unique<StructuredBuffer>(CD3DX12_RESOURCE_DESC::Buffer(totalSize, uavFlags), totalGrids, particleSize, L"Particle_OutPut");

	std::vector<Particle> particlebuffer(totalGrids);

	float dx = cloth.size.x / (cloth.gridsize.x - 1);
	float dy = cloth.size.y / (cloth.gridsize.y - 1);

	XMMATRIX translation = XMMatrixTranslation(-cloth.size.x / 2.0f, 2.0f, -cloth.size.y / 2.0f);
	for (int y = 0; y < cloth.gridsize.y; ++y)
	{
		for (int x = 0; x < cloth.gridsize.x; ++x)
		{
			particlebuffer[y + x * cloth.gridsize.y].pos = XMVector3Transform(XMLoadFloat3(&XMFLOAT3(dx * x, 0.f, dy * y)), translation);
			particlebuffer[y + x * cloth.gridsize.y].vel = XMVectorZero();
		}
	}

	for(int i = 0; i < 4; ++i)
	{
		particlebuffer[900 * i].pinned = 1.f;
	}

	cmdList.CopyStructuredBuffer(*mVertexInput, particlebuffer);

	std::vector<uint32_t> indices;
	for (uint32_t y = 0; y < cloth.gridsize.y - 1; ++y)
	{
		for (uint32_t x = 0; x < cloth.gridsize.x; ++x)
		{
			indices.push_back((y + 1) * cloth.gridsize.x + x);
			indices.push_back((y)*cloth.gridsize.x + x);
		}
		indices.push_back(0xFFFFFFFF);
	}

	mClothIndexNum = indices.size();
	cmdList.CopyIndexBuffer(mIndexBuffer, indices);
	mIndexBuffer.CreateIndexBufferView(mClothIndexNum, sizeof(uint32_t));

	mVertexBufferView.BufferLocation = mVertexOutput->GetD3D12Resource()->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = particlebuffer.size() * sizeof(Particle);
	mVertexBufferView.StrideInBytes = sizeof(Particle);

	computeDatas.restDistH = dx;
	computeDatas.restDistV = dy;
	computeDatas.restDistD = sqrtf(dx * dx + dy * dy);
	computeDatas.particleCount = cloth.gridsize;
	computeDatas.deltaT = 0.000005f;

	ResourceStateTracker::AddGlobalResourceState(mVertexInput->GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON);
	ResourceStateTracker::AddGlobalResourceState(mVertexOutput->GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON);
}

void RigidBodyDemo::ComputeCall(CommandList& cmdList)
{
	//Graphis to Compute Barrier
	int readSet = 1;
	const int iteration = 65;
	int calcNormal = 0;

	cmdList.SetPipelineState(mComputePass->mPSO);
	cmdList.SetComputeRootSignature(mComputePass->mRootSig);
	cmdList.SetComputeDynamicConstantBuffer(2, computeDatas);
	cmdList.SetCompute32BitConstants(3, calcNormal);

	for (int i = 0; i < iteration; ++i)
	{
		readSet = 1 - readSet;
		if (readSet)
		{
			cmdList.SetComputeRootSRV(0, mVertexOutput->GetD3D12Resource()->GetGPUVirtualAddress());
			cmdList.SetComputeRootUAV(1, mVertexInput->GetD3D12Resource()->GetGPUVirtualAddress());
		}
		else
		{
			cmdList.SetComputeRootSRV(0, mVertexInput->GetD3D12Resource()->GetGPUVirtualAddress());
			cmdList.SetComputeRootUAV(1, mVertexOutput->GetD3D12Resource()->GetGPUVirtualAddress());
		}

		if (i == iteration - 1)
		{
			calcNormal = 1;
			cmdList.SetCompute32BitConstants(3, calcNormal);
		}
		cmdList.SetCompute32BitConstants(4, mRemovePin);
		cmdList.Dispatch(cloth.gridsize.x / 10, cloth.gridsize.y / 10, 1);
		if (i != iteration - 1)
		{
			//Compute To Compute Barrier
			cmdList.UAVBarrier(*mVertexInput);
			cmdList.UAVBarrier(*mVertexOutput);
		}
	}
}

void RigidBodyDemo::ClothRenderCall(CommandList& cmdList)
{
	auto rtvCpuHandle = mDescriptorHeaps[HeapType::RTV]->GetCpuHandle(mRtvIdx);
	auto dsvCpuHandle = mDescriptorHeaps[HeapType::DSV]->GetCpuHandle(mDsvIdx);

	float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	cmdList.ClearTexture(mRenderTarget.GetTexture(AttachmentPoint::Color0), rtvCpuHandle, clearColor);
	cmdList.ClearDepthStencilTexture(mRenderTarget.GetTexture(AttachmentPoint::DepthStencil), dsvCpuHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL);

	cmdList.SetPipelineState(mDrawPass->mPSO);
	cmdList.SetGraphicsRootSignature(mDrawPass->mRootSig);

	cmdList.SetSingleRenderTarget(&rtvCpuHandle, &dsvCpuHandle);

	cmdList.SetViewport(m_Viewport);
	cmdList.SetScissorRect(m_ScissorRect);

	cmdList.SetGraphics32BitConstants(0, mViewProjection);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList.SetVertexBuffer(0, mVertexBufferView);
	cmdList.SetIndexBuffer(mIndexBuffer);
	cmdList.DrawIndexed(mClothIndexNum);
}

void RigidBodyDemo::ObjectRenderCall(CommandList& cmdList)
{
	auto rtvCpuHandle = mDescriptorHeaps[HeapType::RTV]->GetCpuHandle(mRtvIdx);
	auto dsvCpuHandle = mDescriptorHeaps[HeapType::DSV]->GetCpuHandle(mDsvIdx);

	cmdList.SetPipelineState(mForwardPass->mPSO);
	cmdList.SetGraphicsRootSignature(mForwardPass->mRootSig);

	cmdList.SetViewport(m_Viewport);
	cmdList.SetScissorRect(m_ScissorRect);

	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList.SetGraphics32BitConstants(0, mViewProjection);
	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), &mCommonCB);

	cmdList.SetSingleRenderTarget(&rtvCpuHandle, &dsvCpuHandle);

	mSphere->Draw(cmdList);
}

void RigidBodyDemo::UpdateConstantBuffer(UpdateEventArgs& e)
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

	mViewProjection = mCommonCB.ViewProj;
}

void RigidBodyDemo::OnUpdate(UpdateEventArgs& e)
{
	IDemo::OnUpdate(e);
	UpdateConstantBuffer(e);
	mCamera.Update(e);
	StartGuiFrame();
	UpdateGui();
}

void RigidBodyDemo::OnRender(RenderEventArgs& e)
{
	IDemo::OnRender(e);
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	ComputeCall(*cmdList);
	ClothRenderCall(*cmdList);
	ObjectRenderCall(*cmdList);
	DrawGui(*cmdList);

	commandQueue->ExecuteCommandList(cmdList);
	m_pWindow->Present(mRenderTarget.GetTexture(AttachmentPoint::Color0));
}

void RigidBodyDemo::OnMouseMoved(MouseMotionEventArgs& e)
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse == false)
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
}

void RigidBodyDemo::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
	mLastMousePos.x = e.X;
	mLastMousePos.y = e.Y;

	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0, true);
}

void RigidBodyDemo::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0, false);
}

void RigidBodyDemo::InitGui()
{
	auto device = DxEngine::Get().GetDevice();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO GuiIO = ImGui::GetIO(); (void)GuiIO;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(DxEngine::Get().GetWindowHandle());

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mGuiSrvUavCbvHeap)) != S_OK)
	{
		assert("Failed to create IMGUI SRV heap");
	}

	ImGui_ImplDX12_Init(device.Get(), 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, mGuiSrvUavCbvHeap.Get(),
		mGuiSrvUavCbvHeap->GetCPUDescriptorHandleForHeapStart(),
		mGuiSrvUavCbvHeap->GetGPUDescriptorHandleForHeapStart());
}

void RigidBodyDemo::StartGuiFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void RigidBodyDemo::UpdateGui()
{
	ImGui::Begin("GUI");
	if(ImGui::Button("Remove Pins"))
	{
		mRemovePin = 1.0f;
	}
	ImGui::End();
}

void RigidBodyDemo::ClearGui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void RigidBodyDemo::DrawGui(CommandList& cmdList)
{
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList.SetDescriptorHeap(mGuiSrvUavCbvHeap);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.GetGraphicsCommandList().Get());
}
