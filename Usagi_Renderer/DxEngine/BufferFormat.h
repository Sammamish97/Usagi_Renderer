#pragma once
#include <d3d12.h>

enum class BufferType
{
	SWAPCHAIN,
	DEPTH_STENCIL_DSV
};

class BufferFormat
{
public:
	static DXGI_FORMAT GetBufferFormat(BufferType type)
	{
		switch (type)
		{
		case BufferType::SWAPCHAIN:
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		case BufferType::DEPTH_STENCIL_DSV:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
};