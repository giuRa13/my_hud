/*
- implement the DirectX 11 SwapChain Present Hook, 
- map into LMU's shared memory, 
- and draw widgets 
*/
#include <windows.h>
#include <tchar.h> // REQUIRED for _T() macro
#include <d3d11.h>
#include <dxgi.h>
#include <fstream>
#include <imgui.h>
#include <shared/config.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <exe/gui_layer.h> 
#include <shared/widgets.h>
#include <shared/texture_loader.h>
#include <dll/lmu_telemetry.h>
#include <dll/lap_history.h>
#include <MinHook.h>

WNDPROC o_WndProc = nullptr;
HWND g_hWnd = nullptr;
bool g_Initialized = false;
static HMODULE g_hModule = nullptr;

// function pointer for the original swap chain present method
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* p_SwapChain, UINT SyncInterval, UINT Flags);
IDXGISwapChainPresent o_Present = nullptr;

ID3D11Device* p_Device = nullptr;
ID3D11DeviceContext* p_Context = nullptr;
ID3D11RenderTargetView* main_RenderTargetView = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    return CallWindowProc(o_WndProc, hWnd, msg, wParam, lParam);
}

// hooked Present function running inside LMU's render loop
HRESULT __stdcall hkPresent(IDXGISwapChain* p_SwapChain, UINT SyncInterval, UINT Flags)
{
    // read configuration live from disk every frame
    config::AppConfig config = config::load_config("config.json", g_hModule);

    // one time init block
    if (!g_Initialized)
    {
        // know the hook is actively executing inside LMU!
        //::MessageBoxA(NULL, "hkPresent Hook Triggered!", "LMU Overlay Debug", MB_OK);

        if (SUCCEEDED(p_SwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&p_Device)))
        {
            p_Device->GetImmediateContext(&p_Context);

            widgets::g_texture_device  = p_Device;  
            widgets::g_texture_context = p_Context;

            DXGI_SWAP_CHAIN_DESC sd;
            p_SwapChain->GetDesc(&sd);

            g_hWnd = sd.OutputWindow;
            // hook WndProc to capture mouse/keyboard inputs
            if (g_hWnd && !o_WndProc) 
            {
                o_WndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            }

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(p_Device, p_Context);
            ImGui::StyleColorsDark();

            // create Render Target View for the backbuffer
            ID3D11Texture2D* pBackBuffer;
            p_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            if (pBackBuffer) 
            {
                p_Device->CreateRenderTargetView(pBackBuffer, NULL, &main_RenderTargetView);
                pBackBuffer->Release();
            }

            g_Initialized = true;
        }
        else 
        {
            return o_Present(p_SwapChain, SyncInterval, Flags);
        }
    }

    // overlay off: release the shared memory handles and skip rendering
    if (!config.enable_overlay) 
    {
        LMUTelemetry::get().shutdown(); // cheap, only null checks when already closed
        return o_Present(p_SwapChain, SyncInterval, Flags);
    }

    // pull telemetry data from LMU memory map 
    LMUTelemetry::get().update();
    LapHistory::get().update();

    // start ImGui Frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // draw
    if (config.show_pedals)  widgets::pedals_widget(config);
    if (config.show_delta)  widgets::delta_widget(config);
    if (config.show_gear)  widgets::gear_widget(config);
    if (config.show_wheel)  widgets::wheel_widget(config);
    if (config.show_lap_history)  widgets::lap_history_widget(config);

    // render ImGui drawing data onto LMU's backbuffer
    ImGui::Render();

    ID3D11RenderTargetView* originalRTV = nullptr;
    p_Context->OMGetRenderTargets(1, &originalRTV, NULL);

    // bind target and render ImGui data safely
    p_Context->OMSetRenderTargets(1, &main_RenderTargetView, NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    if (originalRTV)
    {
        p_Context->OMSetRenderTargets(1, &originalRTV, NULL);
        originalRTV->Release();
    }

    return o_Present(p_SwapChain, SyncInterval, Flags);
}

// helper thread to set up the MinHook pointer grab
DWORD WINAPI MainThread(LPVOID lpParam)
{
    // prove DLLMain spawned the thread successfully
    //::MessageBoxA(NULL, "DLL Attached & MainThread Started", "LMU Overlay Debug", MB_OK);

    if (MH_Initialize() != MH_OK) 
    {
        ::MessageBoxA(NULL, "MinHook Init Failed!", "Error", MB_OK);
        return 1;
    }

    // to get the virtual method table (VMT) address of Present, we create a dummy device/swapchain temporarily
    WNDCLASSEX wc = { 
        sizeof(WNDCLASSEX), 
        CS_CLASSDC, 
        DefWindowProc, 
        0L, 0L, 
        GetModuleHandle(NULL), 
        NULL, NULL, NULL, NULL, 
        _T("DX11Dummy"), 
        NULL
    };
    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow(
        _T("DX11Dummy"), 
        _T(""), 
        WS_OVERLAPPEDWINDOW, 
        100, 100, 300, 300, 
        NULL, NULL, 
        wc.hInstance, 
        NULL
    );

    D3D_FEATURE_LEVEL requested_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL obtained_level;
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    //sd.SwapEffect = DXGI_DXGI_SWAP_EFFECT_DISCARD;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* p_SwapChain = nullptr;
    ID3D11Device* p_DummyDevice = nullptr;
    ID3D11DeviceContext* p_DummyContext = nullptr;

    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, requested_levels, 2,
        D3D11_SDK_VERSION, &sd, &p_SwapChain, &p_DummyDevice, &obtained_level, &p_DummyContext) == S_OK)
    {
        void** p_VTable = *reinterpret_cast<void***>(p_SwapChain);
        LPVOID p_PresentAddress = p_VTable[8]; // Present is index 8 in the DXGI SwapChain VTable

        p_SwapChain->Release();
        p_DummyDevice->Release();
        p_DummyContext->Release();
        DestroyWindow(hWnd);
        UnregisterClass(_T("DX11Dummy"), wc.hInstance);

        // Hook Present using MinHook
        if (MH_CreateHook(p_PresentAddress, reinterpret_cast<LPVOID>(&hkPresent), reinterpret_cast<LPVOID*>(&o_Present)) == MH_OK)
        {
            MH_EnableHook(p_PresentAddress);
            //::MessageBoxA(NULL, "Present Hook Created & Enabled Successfully!", "Success", MB_OK);
        }
        else {
            ::MessageBoxA(NULL, "MH_CreateHook Failed!", "Error", MB_OK);
        }
    }
    else {
        ::MessageBoxA(NULL, "Dummy DX11 Device Creation Failed!", "Error", MB_OK);
    }

    return 0;
}

// DLL Entry Point called when injected into LMU
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule; // captures the DLL's module path correctly!
        LapHistory::get().set_module(hModule);
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        //Clean detachment left completely to Windows process termination. Zero risk of crashes.
        // is common for D3D injection overlays to avoid crashing the game on exit, 
        // Windows automatically cleans up open file mapping handles and unmaps views of files when the DLL unloads or the game closes
        //LMUTelemetry::get().shutdown(); //RISKY
        break;
    }
    return TRUE;
}
