#include "RigidBodyDemo.h"
#include "d3dx12.h"
#include "DxEngine.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ComputePass.h"
#include "DrawPass.h"
#include "DxUtil.h"
#include "ResourceStateTracker.h"

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
	auto device = DxEngine::Get().GetDevice();
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto cmdList = commandQueue->GetCommandList();

	PrepareBuffers(*cmdList);
	BuildComputeDescriptorHeaps();

	mShaders["RigidCS"] = DxUtil::CompileShader(L"../shaders/RigidCompute.hlsl", nullptr, "CS", "cs_5_1");

	mComputePass = std::make_unique<ComputePass>(mShaders["RigidCS"]);

	auto fenceValue = commandQueue->ExecuteCommandList(cmdList);
	commandQueue->WaitForFenceValue(fenceValue);
	return true;
}

void RigidBodyDemo::UnloadContent()
{
	
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

	XMMATRIX translation = XMMatrixTranslation(-cloth.size.x / 2.0f, -2.0f, -cloth.size.y / 2.0f);
	for (int y = 0; y < cloth.gridsize.y; ++y)
	{
		for (int x = 0; x < cloth.gridsize.x; ++x)
		{
			particlebuffer[y + x * cloth.gridsize.y].pos = XMVector3Transform(XMLoadFloat3(&XMFLOAT3(dx * x, 0.f, dy * y)), translation);
			particlebuffer[y + x * cloth.gridsize.y].vel = XMVectorZero();
		}
	}

	cmdList.CopyStructuredBuffer(*mVertexInput, particlebuffer);

	std::vector<uint32_t> indices;
	for (uint32_t y = 0; y < cloth.gridsize.y - 1; ++y)
	{
		for (uint32_t x = 0; x < cloth.gridsize.x - 1; ++x)
		{
			indices.push_back((y + 1) * cloth.gridsize.x + x);
			indices.push_back((y)*cloth.gridsize.x + x);
		}
		indices.push_back(0xFFFFFFFF);
	}
	cmdList.CopyIndexBuffer(mIndexBuffer, indices);

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

void RigidBodyDemo::BuildComputeList()
{
	auto device = DxEngine::Get().GetDevice();
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto mComputeList = commandQueue->GetCommandList();
	//Graphis to Compute Barrier
	int readSet = 1;
	const int iteration = 64;
	int calcNormal = 0;

	mComputeList->SetPipelineState(mComputePass->mPSO);
	mComputeList->SetComputeRootSignature(mComputePass->mRootSig);
	mComputeList->SetComputeDynamicConstantBuffer(2, computeDatas);
	mComputeList->SetCompute32BitConstants(3, calcNormal);
	for (int i = 0; i < 1; ++i)
	{
		readSet = 1 - readSet;
		if(readSet)
		{
			/*mComputeList->SetDescriptorHeap(mComputeDescHeaps[readSet]->GetDescriptorHeap());
			mComputeList->SetComputeRootSRV(0, mVertexOutput->GetD3D12Resource()->GetGPUVirtualAddress());
			mComputeList->SetComputeRootUAV(1, mVertexInput->GetD3D12Resource()->GetGPUVirtualAddress());*/
		}
		else
		{
			mComputeList->SetDescriptorHeap(mComputeDescHeaps[readSet]->GetDescriptorHeap());
			mComputeList->SetComputeRootSRV(0, mVertexInput->GetD3D12Resource()->GetGPUVirtualAddress());
			mComputeList->SetComputeRootUAV(1, mVertexOutput->GetD3D12Resource()->GetGPUVirtualAddress());
		}
		
		if (i == iteration - 1)
		{
			calcNormal = 1;
			mComputeList->SetCompute32BitConstants(3, calcNormal);
		}

		mComputeList->Dispatch(cloth.gridsize.x / 10, cloth.gridsize.y / 10, 1);
		if (i != iteration - 1)
		{
			//Compute To Compute Barrier
			mComputeList->UAVBarrier(*mVertexInput);
			mComputeList->UAVBarrier(*mVertexOutput);
		}
	}
	mComputeList->UAVBarrier(*mVertexInput);
	mComputeList->UAVBarrier(*mVertexOutput);
	//Compute To Graphics Barrier
	//Close buffer.
	auto fenceValue = commandQueue->ExecuteCommandList(mComputeList);
}

void RigidBodyDemo::BuildGraphicsList()
{
	auto device = DxEngine::Get().GetDevice();
	auto commandQueue = DxEngine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);


}

void RigidBodyDemo::BuildComputeDescriptorHeaps()
{
	auto descSize = DxEngine::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mComputeDescHeaps[0] = std::make_unique<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descSize, 3);
	mComputeDescHeaps[1] = std::make_unique<DescriptorHeap>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descSize, 3);
	D3D12_BUFFER_SRV buffer;
	buffer.FirstElement = 0;
	buffer.NumElements = mVertexInput->GetNumElements();
	buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	buffer.StructureByteStride = mVertexInput->GetElementSize();

	D3D12_BUFFER_UAV uav_buffer;
	uav_buffer.FirstElement = 0;
	uav_buffer.NumElements = mVertexInput->GetNumElements();
	uav_buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uav_buffer.StructureByteStride = mVertexInput->GetElementSize();
	uav_buffer.CounterOffsetInBytes = 0;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = mVertexInput->GetD3D12ResourceDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer = buffer;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = mVertexInput->GetD3D12ResourceDesc().Format;
	uavDesc.Buffer = uav_buffer;

	DxEngine::Get().CreateSrvDescriptor(srvDesc, mVertexInput.get()->GetD3D12Resource(), mComputeDescHeaps[0]->GetCpuHandle(0));
	DxEngine::Get().CreateUavDescriptor(uavDesc, mVertexOutput.get()->GetD3D12Resource(), mComputeDescHeaps[0]->GetCpuHandle(1));

	DxEngine::Get().CreateSrvDescriptor(srvDesc, mVertexOutput.get()->GetD3D12Resource(), mComputeDescHeaps[1]->GetCpuHandle(0));
	DxEngine::Get().CreateUavDescriptor(uavDesc, mVertexInput.get()->GetD3D12Resource(), mComputeDescHeaps[1]->GetCpuHandle(1));
}

void RigidBodyDemo::Draw(CommandList& cmdList)
{
	//Run Compute
}

void RigidBodyDemo::OnUpdate(UpdateEventArgs& e)
{
	
}

void RigidBodyDemo::OnRender(RenderEventArgs& e)
{
	BuildComputeList();

}