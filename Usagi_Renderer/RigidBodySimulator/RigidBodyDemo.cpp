#include "RigidBodyDemo.h"
#include "d3dx12.h"
#include "DxEngine.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "ComputePass.h"
#include "DxUtil.h"

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
}

void RigidBodyDemo::Draw(CommandList& cmdList)
{
	cmdList.SetComputeRootSignature(mComputePass->mRootSig);
	cmdList.SetDescriptorHeap(mDemoSrvUavCbvHeap);

	cmdList.SetPipelineState(mComputePass->mPSO);
	
	for (int i = 0; i < 64; ++i)
	{
		cmdList.Dispatch(cloth.gridsize.x / 10, cloth.gridsize.y / 10, 1);
	}
}