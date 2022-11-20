// IK_Demo.cpp : Defines the entry point for the application.
//
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <dxgidebug.h>

#include "framework.h"
#include "IKDemo.h"
#include "DxEngine.h"

//void ReportLiveObjects()
//{
//    IDXGIDebug1* dxgiDebug;
//    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
//
//    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
//    dxgiDebug->Release();
//}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    WCHAR path[MAX_PATH];

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
        {
            // -wd Specify the Working Directory.
            if (wcscmp(argv[i], L"-wd") == 0)
            {
                wcscpy_s(path, argv[++i]);
                SetCurrentDirectoryW(path);
            }
        }
        LocalFree(argv);
    }

    DxEngine::Create(hInstance);
    {
        std::shared_ptr<IKDemo> demo = std::make_shared<IKDemo>(L"Ussagi_Renderer: IK Demo", 1280, 720);
        retCode = DxEngine::Get().Run(demo);
    }
    DxEngine::Destroy();

    //atexit(&ReportLiveObjects);

    return retCode;
}