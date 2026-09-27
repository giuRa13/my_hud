#include <exe/app_layer.h>
#include <tchar.h>
#include <shared/config.h> 

// forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

App* App::instance = nullptr;

App::App()
{
    instance = this;
}

bool App::init()
{
    wc = { 
        sizeof(wc), 
        CS_CLASSDC, 
        App::WndProc, 
        0L, 0L, 
        GetModuleHandle(NULL), 
        NULL, NULL, NULL, NULL,
        L"BMY_HUD_Panel",
        NULL
    };
    ::RegisterClassExW(&wc);

    hwnd = ::CreateWindowW(
        wc.lpszClassName, 
        L"LMU Overlay Control Panel", 
        WS_OVERLAPPEDWINDOW, 
        100, 100, 
        1024, 640, //1024, 768, 
        NULL, NULL, 
        wc.hInstance, 
        NULL
    );

    // init Direct3D
    if(!create_DeviceD3D())
    {
        cleanup_DeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    gui_layer.init(hwnd, pd3dDevice);

    return true;
}

void App::run()
{
    MSG msg;
    ::ZeroMemory(&msg, sizeof(msg));

    while (running && msg.message != WM_QUIT)
    {
        // poll and handle messages (inputs, window resize, etc.)
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        if (gui_layer.get_vsync_modified())
        {
            d3dpp.PresentationInterval = gui_layer.get_vsync() 
                ? D3DPRESENT_INTERVAL_ONE 
                : D3DPRESENT_INTERVAL_IMMEDIATE;

            reset_Device();
            // clear the flag so we don't reset every frame
            gui_layer.get_vsync_modified() = false;
        }

        gui_layer.begin();

        // render graphics Background/Clear
        pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        D3DCOLOR clear_col = D3DCOLOR_RGBA(0, 0, 0, (int)(gui_layer.get_opacity() * 255.0f));
        pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col, 1.0f, 0);

        if (pd3dDevice->BeginScene() >= 0)
        {
            // end frame & draw ImGui
            gui_layer.end();
            pd3dDevice->EndScene();
        }

        // handle device loss
        HRESULT result = pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST && pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        {
            reset_Device();
        }
    }
}

void App::shutdown()
{
    gui_layer.shutdown();
    cleanup_DeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

bool App::create_DeviceD3D()
{
    if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) 
        return false;

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // need to use an explicit format with alpha if needing per-pixel alpha on the window
    //d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8; 
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // present without vsync

    if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice) < 0)
        return false;

    return true;
}

void App::cleanup_DeviceD3D()
{
    if (pd3dDevice) { pd3dDevice->Release(); pd3dDevice = nullptr; }
    if (pD3D) { pD3D->Release(); pD3D = nullptr; }
}

void App::reset_Device()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = pd3dDevice->Reset(&d3dpp);
    if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI App::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (instance)
        return instance->handle_Message(hWnd, msg, wParam, lParam);

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT App::handle_Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            d3dpp.BackBufferWidth = LOWORD(lParam);
            d3dpp.BackBufferHeight = HIWORD(lParam);
            reset_Device();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        running = false;
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}