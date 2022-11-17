#include "IKDemo.h"
#include "DxEngine.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "Object.h"
#include "Window.h"

IKDemo::IKDemo(const std::wstring& name, int width, int height, bool vSync)
	:IDemo(name, width, height, vSync)
{
	//Initialize PSO, RootSignature, Descriptors, ETC...

}

bool IKDemo::LoadContent()
{
	//Model, Shader, Texure Load
	return true;
}

void IKDemo::UnloadContent()
{

}

void IKDemo::InitPSO()
{
	//TODO
}

void IKDemo::InitRootSignature()
{
	//TODO
}

void IKDemo::InitDescriptors()
{
	//TODO
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

	auto rtvCpuHandle = 0;
	auto dsvCpuHandle = 0;

	float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

	//TODO
	//cmdList->ClearTexture();
	//cmdList->ClearDepthStencilTexture();
	/*cmdList->SetPipelineState();
	cmdList->SetGraphicsRootSignature();

	cmdList->SetGraphicsDynamicConstantBuffer(1, sizeof(CommonCB), mCommonCB.get());
	cmdList->SetViewport(mScreenViewport);
	cmdList->SetScissorRect(mScissorRect);
	cmdList->SetRenderTargets(rtvArray, &dsvHeapCPUHandle);
	cmdList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (const auto& object : mObjects)
	{
		object->Draw(cmdList);
	}*/
	commandQueue->ExecuteCommandList(cmdList);
	m_pWindow->Present();
}