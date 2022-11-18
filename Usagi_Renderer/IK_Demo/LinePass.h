#pragma once
#include "IPass.h"
class LinePass : public IPass
{
public:
	LinePass(ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};

