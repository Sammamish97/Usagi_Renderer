// DxEngine.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "DxEngine.h"
#include "BufferFormat.h"
#include "Window.h"
#include "IDemo.h"
#include "CommandQueue.h"

#include <cassert>

#include "DxUtil.h"

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";
static std::shared_ptr<Window> gsWindow;
static DxEngine* gsSingleTon = nullptr;

uint64_t DxEngine::msFrameCount = 0;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// A wrapper struct to allow shared pointers for the window class.
// This is needed because the constructor and destructor for the Window
// class are protected and not accessible by the std::make_shared method.
struct MakeWindow : public Window
{
    MakeWindow(HWND hWnd, const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync)
        : Window(hWnd, windowName, clientWidth, clientHeight, vSync)
    {}
};

DxEngine::DxEngine(HINSTANCE hInst)
    :mHinstance(hInst), mTearingSupported(false)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    WNDCLASSEXW wndClass = { 0 };

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = &WndProc;
    wndClass.hInstance = mHinstance;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //wndClass.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = WINDOW_CLASS_NAME;
    //wndClass.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(APP_ICON));

    if (!RegisterClassExW(&wndClass))
    {
        MessageBoxA(NULL, "Unable to register the window class.", "Error", MB_OK | MB_ICONERROR);
    }
}

HWND DxEngine::GetWindowHandle() const
{
    return gsWindow->GetWindowHandle();
}

void DxEngine::RemoveWindow()
{

}

void DxEngine::Initialize()
{
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug1> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
    // Enable these if you want full validation (will slow down rendering a lot).
    //debugInterface->SetEnableGPUBasedValidation(TRUE);
    //debugInterface->SetEnableSynchronizedCommandQueueValidation(TRUE);
#endif
    auto dxgiAdapter = GetAdapter(false);
    if (!dxgiAdapter)
    {
        dxgiAdapter = GetAdapter(true);
    }

    if (dxgiAdapter)
    {
        mDevice = CreateDevice(dxgiAdapter);
    }
    else
    {
        throw std::exception("DXGI adapter enumeration failed.");
    }

    mDirectCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    mComputeCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    mCopyCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

    mTearingSupported = CheckTearingSupport();

    msFrameCount = 0;
}

void DxEngine::Create(HINSTANCE hInst)
{
    if (!gsSingleTon)
    {
        gsSingleTon = new DxEngine(hInst);
        gsSingleTon->Initialize();
    }
}

DxEngine& DxEngine::Get()
{
    assert(gsSingleTon);
    return *gsSingleTon;
}

void DxEngine::Destroy()
{
    if (gsSingleTon)
    {
        //assert(!gsWindow && "All windows should be destroyed before destroying the application instance.");

        delete gsSingleTon;
        gsSingleTon = nullptr;
    }
}

ComPtr<ID3D12Device2> DxEngine::GetDevice() const
{
    return mDevice;
}

DxEngine::~DxEngine()
{
    Flush();
}

std::shared_ptr<Window> DxEngine::CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight,
	bool vSync)
{
    RECT windowRect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowW(WINDOW_CLASS_NAME, windowName.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, mHinstance, nullptr);

    if (!hWnd)
    {
        MessageBoxA(NULL, "Could not create the render window.", "Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }
    gsWindow = std::make_shared<MakeWindow>(hWnd, windowName, clientWidth, clientHeight, vSync);
    return gsWindow;
}

void DxEngine::DestroyWindow(std::shared_ptr<Window> window)
{
    gsWindow->Destroy();
}

std::shared_ptr<Window> DxEngine::GetWindowByName(const std::wstring& windowName)
{
    return gsWindow;
}

int DxEngine::Run(std::shared_ptr<IDemo> pGame)
{
    if (!pGame->Initialize()) return 1;
    if (!pGame->LoadContent()) return 2;

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            ++DxEngine::msFrameCount;
            UpdateEventArgs updateEventArgs(0.0f, 0.0f, DxEngine::msFrameCount);
            gsWindow->OnUpdate(updateEventArgs);

            RenderEventArgs renderEventArgs(0.0f, 0.0f, DxEngine::msFrameCount);
            gsWindow->OnRender(renderEventArgs);
        }
    }

    // Flush any commands in the commands queues before quiting.
    Flush();

    pGame->UnloadContent();
    pGame->Destroy();

    return static_cast<int>(msg.wParam);
}

void DxEngine::Quit(int exitCode)
{
    PostQuitMessage(exitCode);
}

void DxEngine::Flush()
{
    mDirectCommandQueue->Flush();
    mComputeCommandQueue->Flush();
    mCopyCommandQueue->Flush();
}

ComPtr<ID3D12DescriptorHeap> DxEngine::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

bool DxEngine::IsTearingSupported() const
{
    return mTearingSupported;
}

DXGI_SAMPLE_DESC DxEngine::GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples,
	D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags) const
{
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format = format;
    qualityLevels.SampleCount = 1;
    qualityLevels.Flags = flags;
    qualityLevels.NumQualityLevels = 0;

    while (qualityLevels.SampleCount <= numSamples && SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS))) && qualityLevels.NumQualityLevels > 0)
    {
        // That works...
        sampleDesc.Count = qualityLevels.SampleCount;
        sampleDesc.Quality = qualityLevels.NumQualityLevels - 1;

        // But can we do better?
        qualityLevels.SampleCount *= 2;
    }

    return sampleDesc;
}

std::shared_ptr<CommandQueue> DxEngine::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
    std::shared_ptr<CommandQueue> commandQueue;
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        commandQueue = mDirectCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        commandQueue = mComputeCommandQueue;
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        commandQueue = mCopyCommandQueue;
        break;
    default:
        assert(false && "Invalid command queue type.");
    }

    return commandQueue;
}

UINT DxEngine::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
    return mDevice->GetDescriptorHandleIncrementSize(type);
}

void DxEngine::CreateRtvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    mDevice->CreateRenderTargetView(resource.Get(), &rtvDesc, heapPos);
}

void DxEngine::CreateDsvDescriptor(DXGI_FORMAT format, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = format;
    dsvDesc.Texture2D.MipSlice = 0;
    mDevice->CreateDepthStencilView(resource.Get(), &dsvDesc, heapPos);
}

void DxEngine::CreateCbvDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuLocation, size_t bufferSize, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = gpuLocation;
    cbvDesc.SizeInBytes = bufferSize;
    mDevice->CreateConstantBufferView(&cbvDesc, heapPos);
}

void DxEngine::CreateSrvDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
    mDevice->CreateShaderResourceView(resource.Get(), &srvDesc, heapPos);
}

void DxEngine::CreateUavDescriptor(D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc, ComPtr<ID3D12Resource> resource, D3D12_CPU_DESCRIPTOR_HANDLE heapPos)
{
    mDevice->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, heapPos);
}

ComPtr<IDXGIAdapter4> DxEngine::GetAdapter(bool bUseWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (bUseWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory
            // is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                    D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }

    return dxgiAdapter4;
}

ComPtr<ID3D12Device2> DxEngine::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> d3d12Device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
    //    NAME_D3D12_OBJECT(d3d12Device2);

        // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,				// This started happening after updating to an RTX 2080 Ti. I believe this to be an error in the validation layer itself.
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

bool DxEngine::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing, sizeof(allowTearing));
        }
    }

    return allowTearing == TRUE;
}

MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (messageID)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Left;
    }
    break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Right;
    }
    break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Middel;
    }
    break;
    }

    return mouseButton;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    {
        return true;
    }

    auto pWindow = gsWindow;
    if (gsWindow)
    {
        switch (message)
        {
        case WM_PAINT:
        break;
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            MSG charMsg;
            // Get the Unicode character (UTF-16)
            unsigned int c = 0;
            // For printable characters, the next message will be WM_CHAR.
            // This message contains the character code we need to send the KeyPressed event.
            // Inspired by the SDL 1.2 implementation.
            if (PeekMessageW(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
            {
                GetMessage(&charMsg, hwnd, 0, 0);
                c = static_cast<unsigned int>(charMsg.wParam);

                /*if (charMsg.wParam > 0 && charMsg.wParam < 0x10000)
                    ImGui::GetIO().AddInputCharacter((unsigned short)charMsg.wParam);*/
            }
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
            pWindow->OnKeyPressed(keyEventArgs);
        }
        break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key = (KeyCode::Key)wParam;
            unsigned int c = 0;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

            // Determine which key was released by converting the key code and the scan code
            // to a printable character (if possible).
            // Inspired by the SDL 1.2 implementation.
            unsigned char keyboardState[256];
            GetKeyboardState(keyboardState);
            wchar_t translatedCharacters[4];
            if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
            {
                c = translatedCharacters[0];
            }

            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
            pWindow->OnKeyReleased(keyEventArgs);
        }
        break;
        // The default window procedure will play a system notification sound 
        // when pressing the Alt+Enter keyboard combination if this message is 
        // not handled.
        case WM_SYSCHAR:
            break;
        case WM_MOUSEMOVE:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseMoved(mouseMotionEventArgs);
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseButtonPressed(mouseButtonEventArgs);
        }
        break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
            pWindow->OnMouseButtonReleased(mouseButtonEventArgs);
        }
        break;
        case WM_MOUSEWHEEL:
        {
            // The distance the mouse wheel is rotated.
            // A positive value indicates the wheel was rotated to the right.
            // A negative value indicates the wheel was rotated to the left.
            float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
            short keyStates = (short)LOWORD(wParam);

            bool lButton = (keyStates & MK_LBUTTON) != 0;
            bool rButton = (keyStates & MK_RBUTTON) != 0;
            bool mButton = (keyStates & MK_MBUTTON) != 0;
            bool shift = (keyStates & MK_SHIFT) != 0;
            bool control = (keyStates & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            // Convert the screen coordinates to client coordinates.
            POINT clientToScreenPoint;
            clientToScreenPoint.x = x;
            clientToScreenPoint.y = y;
            ScreenToClient(hwnd, &clientToScreenPoint);

            MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
            pWindow->OnMouseWheel(mouseWheelEventArgs);
        }
        break;
        case WM_SIZE:
        {
            int width = ((int)(short)LOWORD(lParam));
            int height = ((int)(short)HIWORD(lParam));

            ResizeEventArgs resizeEventArgs(width, height);
            pWindow->OnResize(resizeEventArgs);
        }
        break;
        case WM_DESTROY:
        {
            if (gsWindow == nullptr)
            {
                // If there are no more windows, quit the application.
                PostQuitMessage(0);
            }
            PostQuitMessage(0);
        }
        break;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
    else
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}