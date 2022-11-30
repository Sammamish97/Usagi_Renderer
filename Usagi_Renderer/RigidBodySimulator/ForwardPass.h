#pragma once
#include "IPass.h"
class ForwardPass : public IPass
{
public:
	ForwardPass(ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};

