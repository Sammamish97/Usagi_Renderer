#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>

class DxEngine
{
public:
	static DxEngine& Instance();
	static void Destroy();

private:
	DxEngine() = default;
	~DxEngine() = default;

public:

private:


private:
	static DxEngine* singleton_instance;
};

