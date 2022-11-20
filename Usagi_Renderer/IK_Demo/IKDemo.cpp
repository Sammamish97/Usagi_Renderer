#include "IKDemo.h"
#include "DxEngine.h"
#include "DxUtil.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "Object.h"
#include "Window.h"
#include "BufferFormat.h"
#include "Model.h"
#include "BoneModel.h"
#include <math.h>
#include <IMGUI/imgui_impl_dx12.h>
#include <IMGUI/imgui_impl_win32.h>
#include <stack>

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
	InitGui();
	InitDescriptorHeaps();
	
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	mModels["Plane"] = std::make_shared<Model>("../models/Plane.obj", *cmdList);
	mModels["Sphere"] = std::make_shared<Model>("../models/Sphere.obj", *cmdList);

	mShaders["ForwardVS"] = DxUtil::CompileShader(L"../shaders/Forward.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ForwardPS"] = DxUtil::CompileShader(L"../shaders/Forward.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["LineVS"] = DxUtil::CompileShader(L"../shaders/Line.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["LinePS"] = DxUtil::CompileShader(L"../shaders/Line.hlsl", nullptr, "PS", "ps_5_1");

	InitBoneObject();
	mObjects.push_back(std::make_shared<Object>(mModels["Plane"], XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1)));
	mTarget = std::make_shared<Object>(mModels["Sphere"], XMFLOAT3(0, 0, 0), XMFLOAT3(1, 1, 1), XMFLOAT3(0.1, 0.1, 0.1));

	mForwardPass = std::make_unique<ForwardPass>(mShaders["ForwardVS"], mShaders["ForwardPS"]);
	mLinePass = std::make_unique<LinePass>(mShaders["LineVS"], mShaders["LinePS"]);

	InitDescriptorHeaps();
	InitRenderTarget();

	auto fenceValue = commandQueue->ExecuteCommandList(cmdList);
	commandQueue->WaitForFenceValue(fenceValue);

	return true;
}

void IKDemo::UnloadContent()
{
	ClearGui();
}

void IKDemo::OnUpdate(UpdateEventArgs& e)
{
	IDemo::OnUpdate(e);
	StartGuiFrame();
	UpdateGui();
	mCamera.Update(e);
	UpdateConstantBuffer(e);
	UpdateTargetPos();
	//UpdateIkObject();
	//UpdateHirarchyTest();
	UpdateIkObject2();
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

	DrawObject(*cmdList, &rtvCpuHandle, &dsvCpuHandle);
	DrawLine(*cmdList, &rtvCpuHandle, &dsvCpuHandle);
	DrawGui(*cmdList);
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

void IKDemo::InitBoneObject(int linkNum)
{
	mJointNum = linkNum + 1;
	mEEIdx = linkNum;
	
	auto offset = XMFLOAT4(0.f, 2.f, 0.f, 0.f);
	XMVECTOR jointOffset = XMLoadFloat4(&offset);

	mJointMats.push_back(XMMatrixIdentity());

	for (int i = 0; i < linkNum; ++i)
	{
		mJointMats.push_back(XMMatrixTranslationFromVector(jointOffset));
	}
}

void IKDemo::InitDescriptorHeaps()
{
	mDescriptorHeaps[HeapType::RTV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV), 128);
	mDescriptorHeaps[HeapType::DSV] = std::make_shared<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV), 128);
}

void IKDemo::DrawObject(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle)
{
	cmdList.SetPipelineState(mForwardPass->mPSO);
	cmdList.SetGraphicsRootSignature(mForwardPass->mRootSig);

	cmdList.SetViewport(m_Viewport);
	cmdList.SetScissorRect(m_ScissorRect);

	cmdList.SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), &mCommonCB);
	cmdList.SetGraphicsDynamicConstantBuffer(2, sizeof(DirectLight), &mLightCB);

	cmdList.SetSingleRenderTarget(rtvHandle, dsvHandle);

	mTarget->Draw(cmdList);
	for (const auto& object : mObjects)
	{
		object->Draw(cmdList);
	}
}

void IKDemo::DrawLine(CommandList& cmdList, D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle)
{
	cmdList.SetPipelineState(mLinePass->mPSO);
	cmdList.SetGraphicsRootSignature(mLinePass->mRootSig);

	cmdList.SetViewport(m_Viewport);
	cmdList.SetScissorRect(m_ScissorRect);

	cmdList.SetGraphicsDynamicConstantBuffer(0, sizeof(CommonCB), &mCommonCB);

	cmdList.SetSingleRenderTarget(rtvHandle, dsvHandle);
	DrawIkBone(cmdList);
}

void IKDemo::DrawIkBone(CommandList& cmdList)
{
	std::vector<XMVECTOR> absolutePos;
	XMMATRIX lastMat = mJointMats[0];
	absolutePos.push_back(GetDecomposedTranslation(lastMat));
	
	for (int i = 1; i < mJointNum; ++i)
	{
		auto currentMat = mJointMats[i] * lastMat;
		lastMat = currentMat;
		absolutePos.push_back(GetDecomposedTranslation(currentMat));
	}

	std::vector<XMVECTOR> vertices;
	for (int i = 0; i < mJointNum - 1; ++i)
	{
		vertices.push_back(absolutePos[i]);
		vertices.push_back(absolutePos[i+1]);
	}

	cmdList.SetDynamicVertexBuffer(0, vertices);
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList.Draw(vertices.size());
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

void IKDemo::UpdateTargetPos()
{
	mTarget->SetPosition(XMFLOAT3(mTargetPosition.x, mTargetPosition.y, mTargetPosition.z));
}

void IKDemo::UpdateIkObject()
{
	XMVECTOR targetPosition = XMVectorZero();
	XMVECTOR absEEPos = XMVectorZero();
	XMVECTOR lastAbsEEPos = XMVectorZero();

	float threshold = 0.001f;
	breakButton = false;

	while ((GetDistance(targetPosition, absEEPos) > threshold))
	{
		std::stack<XMMATRIX> matStack;
		XMMATRIX lastMat = mJointMats[0];
		matStack.push(lastMat);
		for (int i = 1; i < mJointNum; ++i)
		{
			XMMATRIX currentMat = lastMat * mJointMats[i];
			matStack.push(currentMat);
			lastMat = currentMat;
		}

		XMMATRIX EEMat = matStack.top();
		matStack.pop();

		absEEPos = GetDecomposedTranslation(EEMat);
		XMFLOAT3 absEEPosF(0, 0, 0);

		XMStoreFloat3(&absEEPosF, absEEPos);
		targetPosition = XMLoadFloat4(&mTargetPosition);

		//mJointMats: 상대위치
		for (int i = mEEIdx - 1; i >= 0; --i)
		{
			XMMATRIX currentAbsMat = matStack.top();
			matStack.pop();

			XMVECTOR currentAbsPos = GetDecomposedTranslation(currentAbsMat);

			XMVECTOR jointToEE = (XMVectorSubtract(absEEPos, currentAbsPos));
			XMVECTOR jointToDestination = (XMVectorSubtract(targetPosition, currentAbsPos));

			XMVECTOR rotation_axis = (XMVector3Cross(jointToEE, jointToDestination));
			if(XMVector3Equal(rotation_axis, XMVectorZero()) == true)
			{
				continue;
			}
			auto dot = XMVector3Dot(jointToEE, jointToDestination).m128_f32[0];
			auto length = XMVector3Length(jointToEE).m128_f32[0] * XMVector3Length(jointToDestination).m128_f32[0];
			float rotation_angle = std::acos(dot / length);

			mJointMats[i] = XMMatrixRotationAxis(rotation_axis, rotation_angle) * mJointMats[i];
			for(int j = i + 1 ; j <= mEEIdx; ++j)
			{
				mJointMats[j] = XMMatrixRotationAxis(rotation_axis, rotation_angle) * mJointMats[j];//여기서 translation이 같이 곱해져서 문제가 생긴다. 회전만 적용해야 한다! 
			}

			lastAbsEEPos = absEEPos;
			absEEPos = GetDecomposedTranslation(mJointMats[mEEIdx]);

			if (GetDistance(targetPosition, absEEPos) < threshold)
			{
				//Approach complete.
				break;
			}
		}

		if (GetDistance(lastAbsEEPos, absEEPos) < threshold)
		{
			if (breakButton == false)
			{
				breakButton = true;
			}
			else
			{
				//Can't approach
				break;
			}
		}
		auto distance = GetDistance(targetPosition, absEEPos);
	}

	auto targetTest = targetPosition;
	auto eeTest = absEEPos;
}

void IKDemo::UpdateIkObject2()
{
	//각 joint의 절대좌표를 계산한다. 절대좌표는 각 joint가 회전될때마다 EE부터 root까지 순서대로 바뀐다.
	std::vector<XMMATRIX> absMats;
	XMMATRIX lastMatrix = XMMatrixIdentity();
	for(int i = 0; i < mJointNum; ++i)
	{
		lastMatrix = mJointMats[i] * lastMatrix;
		absMats.push_back(lastMatrix);
	}

	XMVECTOR absEE = GetDecomposedTranslation(absMats[mEEIdx]);
	XMVECTOR lastEE = absEE;
	XMVECTOR targetPos = XMLoadFloat4(&mTargetPosition);
	float successThreshold = 0.1f;
	float exitThreashold = 2.f;
	bool exitButton = false;
	while ((GetDistance(targetPos, absEE) > successThreshold))
	{
		for (int i = mEEIdx - 1; i >= 0; --i)//(EE-1)부터 Root까지 순서대로 회전시킨다.
		{
			//TODO: mTarget을 Object의 좌표계, 그리고 i번째 joint의 좌표계로 가져와야 한다.
			XMVECTOR jointSpaceDestination = targetPos;
			XMVECTOR curJointPos = GetDecomposedTranslation(absMats[i]);

			XMVECTOR jointToEE = XMVectorSubtract(absEE, curJointPos);
			XMVECTOR jointToDestination = XMVectorSubtract(jointSpaceDestination, curJointPos);

			XMVECTOR rotation_axis = XMVector3Cross(jointToEE, jointToDestination);
			if (XMVector3Equal(rotation_axis, XMVectorZero()) == true)
			{
				continue;
			}
			auto dot = XMVector3Dot(jointToEE, jointToDestination).m128_f32[0];
			auto length = XMVector3Length(jointToEE).m128_f32[0] * XMVector3Length(jointToDestination).m128_f32[0];
			float rotation_radian = std::acos(dot / length);

			XMMATRIX curRotMat = XMMatrixRotationAxis(rotation_axis, rotation_radian);

			mJointMats[i + 1] = mJointMats[i + 1] * curRotMat;//(i+1)'th joint is already on the i'th space. Do not need any transform.

			XMMATRIX curInv = GetInverseMatrix(absMats[i]);//Abs to I'th space
			for (int j = i + 2; j < mEEIdx; ++j)//(i+2)번째 좌표계부터 i번째 좌표계로 변환시킨다. 상대 좌표의 특성상 (i+1)은 이미 i번째 좌표계에 있다.  
			{
				XMMATRIX curSpace = absMats[j] * curInv;//abs to i'th space
				XMMATRIX rotatedSpace = curSpace * curRotMat;//Rotate on I'th space

				XMMATRIX absSpace = rotatedSpace * absMats[i];//i'th space to abs space.

				XMMATRIX jointInverse = GetInverseMatrix(absMats[j - 1]);//abs space to (j-1)'th joint space for hirarchy.
				mJointMats[j] = absSpace * jointInverse;//Update j'th joint space which on (j-1) joint space.
			}

			absMats.clear();
			XMMATRIX lastMatrix = XMMatrixIdentity();
			for (int i = 0; i < mJointNum; ++i)
			{
				lastMatrix = mJointMats[i] * lastMatrix;
				absMats.push_back(lastMatrix);
			}
			lastEE = absEE;
			absEE = GetDecomposedTranslation(absMats[mEEIdx]);
		}
		auto result1 = GetDistance(targetPos, absEE);
		auto result2 = GetDistance(lastEE, absEE);
		if(GetDistance(lastEE, absEE) < exitThreashold)
		{
			if(exitButton)
			{
				break;
			}
			exitButton = true;
		}
	}
}

void IKDemo::UpdateHirarchyTest()
{
	XMFLOAT3 temp(1, 1, 1);

	XMVECTOR zero = XMVectorZero();
	XMVECTOR scale = XMLoadFloat3(&temp);

	temp = XMFLOAT3(1, 0, 0);
	XMVECTOR axis = XMLoadFloat3(&temp);

	temp = XMFLOAT3(0, 0.2, 0);

	XMVECTOR translation = XMLoadFloat3(&temp);
	XMVECTOR rotationOrigin = zero;
	XMVECTOR rotationQuat = XMQuaternionRotationAxis(axis, 45.f);
	XMVECTOR rotationIdentity = XMQuaternionIdentity();

	XMVECTOR rootPos = XMLoadFloat4(&mTargetPosition);

	//Root는 World, 나머지는 상대적인 위치이다.
	mJointMats[0] = XMMatrixAffineTransformation(scale, rotationOrigin, rotationIdentity, rootPos);
	mJointMats[1] = XMMatrixAffineTransformation(scale, rotationOrigin, rotationQuat, translation);
	mJointMats[2] = XMMatrixAffineTransformation(scale, rotationOrigin, rotationQuat, translation);
	mJointMats[3] = XMMatrixAffineTransformation(scale, rotationOrigin, rotationQuat, translation);
}

void IKDemo::InitGui()
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

void IKDemo::StartGuiFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void IKDemo::UpdateGui()
{
	static int modelIndex;
	ImGui::Begin("GUI");
	ImGui::SliderFloat3("Target Position", &(mTargetPosition.x), -5, 5);
	ImGui::End();
}

void IKDemo::ClearGui()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void IKDemo::DrawGui(CommandList& cmdList)
{
	cmdList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmdList.SetDescriptorHeap(mGuiSrvUavCbvHeap);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList.GetGraphicsCommandList().Get());
}

XMVECTOR IKDemo::GetDecomposedTranslation(XMMATRIX matrix)
{
	XMVECTOR scale;
	XMVECTOR quat;
	XMVECTOR pos;
	XMMatrixDecompose(&scale, &quat, &pos, matrix);
	return pos;
}

float IKDemo::GetDistance(XMVECTOR lhs, XMVECTOR rhs)
{
	XMFLOAT3 lhs_;
	XMFLOAT3 rhs_;

	XMStoreFloat3(&lhs_, lhs);
	XMStoreFloat3(&rhs_, rhs);

	float x = rhs_.x - lhs_.x;
	float y = rhs_.y - lhs_.y;
	float z = rhs_.z - lhs_.z;

	return std::sqrtf(x * x + y * y + z * z);
}

XMMATRIX IKDemo::GetInverseMatrix(XMMATRIX mat)
{
	XMVECTOR determinant;
	return XMMatrixInverse(&determinant, mat);
}

void IKDemo::OnMouseMoved(MouseMotionEventArgs& e)
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

void IKDemo::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
	mLastMousePos.x = e.X;
	mLastMousePos.y = e.Y;

	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0, true);
}

void IKDemo::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0, false);
}