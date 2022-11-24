#include "RigidBodyDemo.h"
#include "d3dx12.h"
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
	return true;
}

void RigidBodyDemo::UnloadContent()
{

}