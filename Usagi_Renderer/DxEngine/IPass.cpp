#include "pch.h"
#include "IPass.h"

IPass::IPass(ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader)
	:mVertShader(vertShader), mPixelShader(pixelShader), mComputeShader(nullptr)
{

}

IPass::IPass(ComPtr<ID3DBlob> computeShader)
	:mVertShader(nullptr), mPixelShader(nullptr), mComputeShader(computeShader)
{

}