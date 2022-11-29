#include "ComputePass.h"
#include "DxEngine.h"
#include "DxUtil.h"

ComputePass::ComputePass(ComPtr<ID3DBlob> computeShader)
	:IPass(computeShader)
{
	InitRootSignature();
	InitPSO();
}

void ComputePass::InitRootSignature()
{
	//Compute Shader use these values
	//Input vertices structured Buffer SRV
	//Output vertices structured Buffer UAV
	//Common datas: time, mass, spring, gravity, etc...
	//Debug datas: calculate normal
	auto device = DxEngine::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[4];
	rootParameters[0].InitAsShaderResourceView(0);//SRV particle input
	rootParameters[1].InitAsUnorderedAccessView(0);//UAV particle output
	rootParameters[2].InitAsConstantBufferView(0);//Common datas
	rootParameters[3].InitAsConstants(1, 1);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;

	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob))
	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(mRootSig.GetAddressOf())))
}

void ComputePass::InitPSO()
{
	auto device = DxEngine::Get().GetDevice();
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.CS = 
	{
		reinterpret_cast<BYTE*>(mComputeShader->GetBufferPointer()),
		mComputeShader->GetBufferSize()
	};
	computePsoDesc.pRootSignature = mRootSig.Get();
	computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())))
}