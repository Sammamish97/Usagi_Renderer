#pragma once
#include "IPass.h"
class ComputePass : public IPass
{
public:
	ComputePass(ComPtr<ID3DBlob> computeShader);
	void InitRootSignature() override;
	void InitPSO() override;
};

