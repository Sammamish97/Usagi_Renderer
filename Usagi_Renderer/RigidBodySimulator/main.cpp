// IK_Demo.cpp : Defines the entry point for the application.
//
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <dxgidebug.h>

#include "DxEngine.h"
#include "RigidBodyDemo.h"
#include "DxUtil.h"

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    int retCode = 0;

    try
    {
        DxEngine::Create(hInstance);
        {
            std::shared_ptr<RigidBodyDemo> demo = std::make_shared<RigidBodyDemo>(L"Ussagi_Renderer: Rigid Body Demo", 1280, 720);
            retCode = DxEngine::Get().Run(demo);
        }
        DxEngine::Destroy();

        return retCode;
    }
    catch (DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}