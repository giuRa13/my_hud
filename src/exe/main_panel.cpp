#include <exe/app_layer.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    App app;

    if (!app.init())
    {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}

#if 0
#include <windows.h>
#include <d3d9.h>
#include <tchar.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx9.h>

// Forward declare message handler from imgui_impl_win32.cpp (Win32 message handler)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 Application Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};


bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc = { 
        sizeof(wc), 
        CS_CLASSDC, 
        WndProc, 
        0L, 0L, 
        GetModuleHandle(NULL), 
        NULL, NULL, NULL, NULL,
        L"BMY_HUD_Panel",
        NULL
    };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, 
        L"LMU Overlay Control Panel", 
        WS_OVERLAPPEDWINDOW, 
        100, 100, 
        960, 540, 
        NULL, NULL, 
        wc.hInstance, 
        NULL
    );

    // Init Direct3D
    if(!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // show window
    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // setup Dear ImGui ///////////////////////////////////////////////
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // - AddFontDefault() '"'is '"'common, but if you want ForkAwesome or custom fonts, load them here.
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("your_font.ttf", 18.0f);

    bool show_demo_window = true;
    bool enable_pedals = true;
    bool enable_delta = false;
    float opacity = 1.0f;
    static ImVec4 accentColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);

    // Main loop ///////////////////////////////////////////////
    MSG msg;
    ::ZeroMemory(&msg, sizeof(msg));
    while(msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // start ImGui Frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // FULLSCREEN DOCKSPACE SETUP /////////////////////////////////////
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoBringToFrontOnFocus | 
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground; // Makes the host container itself transparent

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("MainDockSpaceHost", NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyMainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // GUI code ///////////////////////////////////////////////////////
            ImGui::Begin("LMU Overlay Settings");

            ImGui::Text("Widgets Configuration");
            ImGui::Checkbox("Pedal Overlay", &enable_pedals);
            ImGui::Checkbox("Live Delta", &enable_delta);
        
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Global Adjustments");
            ImGui::SliderFloat("Overlay Opacity", &opacity, 0.2f, 1.0f);
            if (ImGui::ColorEdit4("Accent Color", &accentColor.x)) 
            {
                ImGuiStyle& style = ImGui::GetStyle();
            
                // 1. Base Accent (Buttons, Sliders, Checkmarks)
                style.Colors[ImGuiCol_Button]             = accentColor;
                style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(accentColor.x + 0.1f, accentColor.y + 0.1f, accentColor.z + 0.1f, accentColor.w);
                style.Colors[ImGuiCol_ButtonActive]       = ImVec4(accentColor.x - 0.1f, accentColor.y - 0.1f, accentColor.z - 0.1f, accentColor.w);
                style.Colors[ImGuiCol_CheckMark]          = accentColor;
                style.Colors[ImGuiCol_SliderGrab]         = accentColor;
                style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(accentColor.x + 0.15f, accentColor.y + 0.15f, accentColor.z + 0.15f, accentColor.w);
                // 2. Frames (ColorEdit inputs, text inputs, dropdowns backgrounds)
                style.Colors[ImGuiCol_FrameBg]            = ImVec4(accentColor.x * 0.3f, accentColor.y * 0.3f, accentColor.z * 0.3f, 0.5f);
                style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(accentColor.x * 0.5f, accentColor.y * 0.5f, accentColor.z * 0.5f, 0.7f);
                style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(accentColor.x * 0.7f, accentColor.y * 0.7f, accentColor.z * 0.7f, 0.9f);
                // 3. Headers (Collapsing headers, selectable items, menu items)
                style.Colors[ImGuiCol_Header]             = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.5f);
                style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.8f);
                style.Colors[ImGuiCol_HeaderActive]       = accentColor;
                // 4. Tabs & Title Bars (The top bars and docking tabs)
                style.Colors[ImGuiCol_Tab]                = ImVec4(accentColor.x * 0.4f, accentColor.y * 0.4f, accentColor.z * 0.4f, 0.86f);
                style.Colors[ImGuiCol_TabHovered]         = ImVec4(accentColor.x * 0.8f, accentColor.y * 0.8f, accentColor.z * 0.8f, 0.80f);
                style.Colors[ImGuiCol_TabActive]          = accentColor;
                style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(accentColor.x * 0.2f, accentColor.y * 0.2f, accentColor.z * 0.2f, 0.97f);
                style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(accentColor.x * 0.5f, accentColor.y * 0.5f, accentColor.z * 0.5f, 1.00f);
                
                style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(accentColor.x * 0.6f, accentColor.y * 0.6f, accentColor.z * 0.6f, 1.00f);
                style.Colors[ImGuiCol_DockingPreview]     = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.7f);
            }

            if(ImGui::Button("Default Color")) ImGui::StyleColorsDark();
        
            ImGui::Spacing();
            if (ImGui::Button("Save Settings & Apply to Game")) 
            {
                // TODO: Save to JSON file here
                // TODO: Signal DLL to reload config
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        
            if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window),

        // End the dockspace host container
        ImGui::End();

        // Rendering ////////////////////////////////////////////
        ImGui::Render();
        LPDIRECT3DDEVICE9 pDevice = g_pd3dDevice;
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col = D3DCOLOR_RGBA(0, 0, 0, (int)(opacity * 255.0f));
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col, 1.0f, 0);
        if (pDevice->BeginScene() >= 0)
        {
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            pDevice->EndScene();
        }
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = pDevice->Present(NULL, NULL, NULL, NULL);

        // handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(NULL);

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) 
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha on the window
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if(g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if((wParam & 0xfff0 == SC_KEYMENU))  // disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
#endif