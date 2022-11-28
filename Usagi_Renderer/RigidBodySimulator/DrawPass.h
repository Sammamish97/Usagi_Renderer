#pragma once
#include "IPass.h"
class DrawPass : public IPass
{
public:
	DrawPass(ComPtr<ID3DBlob> vertShader, ComPtr<ID3DBlob> pixelShader);
	void InitRootSignature() override;
	void InitPSO() override;
};

