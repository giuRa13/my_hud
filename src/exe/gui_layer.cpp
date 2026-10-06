#include <exe/gui_layer.h>
#include <exe/injector.h>
#include <shared/config.h>
#include <shared/widgets.h>
#include <shared/texture_loader.h>
#include <imgui/imgui_internal.h>

void Gui_Layer::init(HWND hwnd, LPDIRECT3DDEVICE9 g_pd3dDevice)
{
    widgets::g_texture_device = g_pd3dDevice;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  

    config = config::load_config("config.json", NULL);

    accent_color = ImVec4(config.accent_r, config.accent_g, config.accent_b, config.accent_a);
    set_theme();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
}

void Gui_Layer::begin()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // dockspace setup
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

    draw_widgets();

    ImGui::End();
}

void Gui_Layer::end()
{
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void Gui_Layer::shutdown()
{
    // tell the DLL (which lives inside LMU) to stop drawing
    config.enable_overlay = false;
    config::save_config(config, "config.json", NULL);

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Gui_Layer::draw_widgets()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Begin("LMU Overlay Settings");

        // globals //////////////////////////////////////////////////////////
        ImGui::Text("Widgets Configuration");
        ImGui::Checkbox("Enable Global Overlay", &config.enable_overlay); 
        ImGui::Checkbox("Design Mode", &config.design_mode); 
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Global Adjustments");
        if (ImGui::Checkbox("Enable V-Sync",  &config.enable_vsync)) 
            vsync_changed = true; // Signal App that need to reset the D3D device

        if (ImGui::SliderFloat("Background Alpha", &config.opacity, 0.0f, 1.0f)) {/*optional extra colors setting*/} 
        
        if (ImGui::ColorEdit4("Accent Color", &accent_color.x)) 
        {
            config.accent_r = accent_color.x;
            config.accent_g = accent_color.y;
            config.accent_b = accent_color.z;
            config.accent_a = accent_color.w;
            apply_accent_color();
        }
        if (ImGui::Button("Default Color")) 
        {
            ImGui::StyleColorsDark();
            accent_color = ImGui::GetStyle().Colors[ImGuiCol_Button];
            config.accent_r = accent_color.x;
            config.accent_g = accent_color.y;
            config.accent_b = accent_color.z;
            config.accent_a = accent_color.w;
        }
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Global Time Colors");
        ImGui::ColorEdit4("Fuchsia (Best)", config.time_fucsia_color);
        ImGui::ColorEdit4("Green (Faster)", config.time_green_color);
        ImGui::ColorEdit4("Yellow (Slower)", config.time_yellow_color);
        ImGui::ColorEdit4("Red (Slower)", config.time_red_color);
        if (ImGui::Button("Reset Defaults"))
        {
            config.time_fucsia_color[0] = 0.956f; config.time_fucsia_color[1] = 0.043f; config.time_fucsia_color[2] = 0.941f; config.time_fucsia_color[3] = 1.0f;
            config.time_green_color[0] = 0.02f; config.time_green_color[1] = 0.9f; config.time_green_color[2] = 0.0f; config.time_green_color[3] = 1.0f;
            config.time_yellow_color[0] = 0.925f; config.time_yellow_color[1] = 0.643f; config.time_yellow_color[2] = 0.047f; config.time_yellow_color[3] = 1.0f;  
            config.time_red_color[0] = 0.898f; config.time_red_color[1] = 0.133f; config.time_red_color[2] = 0.286f; config.time_red_color[3] = 1.0f;
        }
        ImGui::Separator();
        ImGui::Spacing();

        // pedals //////////////////////////////////////////////////////////
        ImGui::Checkbox("Pedals", &config.show_pedals); 
        ImGui::SameLine();
        if (ImGui::Button("Config##Pedals")) 
            show_pedals_settings = !show_pedals_settings;

        // delta //////////////////////////////////////////////////////////
        ImGui::Checkbox("Delta", &config.show_delta);  
        ImGui::SameLine();
        if (ImGui::Button("Config##Delta")) 
            show_delta_settings = !show_delta_settings;

        // gear //////////////////////////////////////////////////////////
        ImGui::Checkbox("Gear", &config.show_gear); 
        ImGui::SameLine();
        if (ImGui::Button("Config##Gear")) 
            show_gear_settings = !show_gear_settings;

        // wheel //////////////////////////////////////////////////////////
        ImGui::Checkbox("Wheel", &config.show_wheel);
        ImGui::SameLine();
        if (ImGui::Button("Config##Wheel"))
            show_wheel_settings = !show_wheel_settings;

        // lap history //////////////////////////////////////////////////////////
        ImGui::Checkbox("Lap History", &config.show_lap_history); 
        ImGui::SameLine();
        if (ImGui::Button("Config##LapHistory")) 
            show_lap_history_settings = !show_lap_history_settings;

        // sectors //////////////////////////////////////////////////////////
        ImGui::Checkbox("Sectors", &config.show_lap_history); 
        ImGui::SameLine();
        if (ImGui::Button("Config##Sectors")) 
            show_sectors_settings = !show_sectors_settings;

        ImGui::Separator();
        ImGui::Spacing();

        // save/apply //////////////////////////////////////////////////////////
        if (ImGui::Button("Save Settings & Apply to Game")) 
            config::save_config(config, "config.json", NULL);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        static std::string statusMessage = "Not Injected";
        static bool is_success_injected = false;

        if (ImGui::Button("Inject Overlay into LMU", ImVec2(200, 30))) 
        {
            char currentDir[MAX_PATH];
            GetModuleFileNameA(NULL, currentDir, MAX_PATH);
            std::string exePath(currentDir);
            std::string binDir = exePath.substr(0, exePath.find_last_of("\\/"));
            std::string dllFullPath = binDir + "\\my_hud.dll";

            statusMessage = inject_DLL(dllFullPath);
            // Check if the returned string starts with "Success"
            is_success_injected = (statusMessage.rfind("Success", 0) == 0);
        }

        ImGui::TextWrapped("%s", statusMessage.c_str());
        
    ImGui::End();

    if (show_demo)ImGui::ShowDemoWindow(&show_demo);

    if (config.design_mode)
    {
        if (config.show_pedals) widgets::pedals_widget(config);
        if (config.show_delta) widgets::delta_widget(config);
        if (config.show_gear) widgets::gear_widget(config);
        if (config.show_wheel)  widgets::wheel_widget(config);
        if (config.show_lap_history)  widgets::lap_history_widget(config);
        if (config.show_sectors)  widgets::sectors_widget(config);
    }

    // settings //////////////////////////////////////////////////////////
    if (show_pedals_settings) widgets::pedals_settings_panel(&show_pedals_settings, config);
    if (show_delta_settings) widgets::delta_settings_panel(&show_delta_settings, config);
    if (show_gear_settings) widgets::gear_settings_panel(&show_gear_settings, config);
    if (show_wheel_settings)  widgets::wheel_settings_panel(&show_wheel_settings, config);
    if (show_lap_history_settings) widgets::lap_history_settings_panel(&show_lap_history_settings, config);
    if (show_sectors_settings) widgets::sectors_settings_panel(&show_sectors_settings, config);
}

void Gui_Layer::set_theme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.TabRounding = 0;
    style.ScrollbarRounding = 8;
    style.WindowRounding = 0;
    style.GrabRounding = 4;
    style.FrameRounding = 0;
    style.PopupRounding = 0;

    apply_accent_color();
}

void Gui_Layer::apply_accent_color()
{
    ImGuiStyle& style = ImGui::GetStyle();
    
    style.Colors[ImGuiCol_Button]             = accent_color;
    style.Colors[ImGuiCol_ButtonHovered]      = ImVec4(accent_color.x + 0.1f, accent_color.y + 0.1f, accent_color.z + 0.1f, accent_color.w);
    style.Colors[ImGuiCol_ButtonActive]       = ImVec4(accent_color.x - 0.1f, accent_color.y - 0.1f, accent_color.z - 0.1f, accent_color.w);
    style.Colors[ImGuiCol_CheckMark]          = accent_color;
    style.Colors[ImGuiCol_SliderGrab]         = accent_color;
    style.Colors[ImGuiCol_SliderGrabActive]   = ImVec4(accent_color.x + 0.15f, accent_color.y + 0.15f, accent_color.z + 0.15f, accent_color.w);
    style.Colors[ImGuiCol_FrameBg]            = ImVec4(accent_color.x * 0.3f, accent_color.y * 0.3f, accent_color.z * 0.3f, 0.5f);
    style.Colors[ImGuiCol_FrameBgHovered]     = ImVec4(accent_color.x * 0.5f, accent_color.y * 0.5f, accent_color.z * 0.5f, 0.7f);
    style.Colors[ImGuiCol_FrameBgActive]      = ImVec4(accent_color.x * 0.7f, accent_color.y * 0.7f, accent_color.z * 0.7f, 0.9f);
    style.Colors[ImGuiCol_Header]             = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.5f);
    style.Colors[ImGuiCol_HeaderHovered]      = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.8f);
    style.Colors[ImGuiCol_HeaderActive]       = accent_color;
    style.Colors[ImGuiCol_Tab]                = ImVec4(accent_color.x * 0.4f, accent_color.y * 0.4f, accent_color.z * 0.4f, 0.86f);
    style.Colors[ImGuiCol_TabHovered]         = ImVec4(accent_color.x * 0.8f, accent_color.y * 0.8f, accent_color.z * 0.8f, 0.80f);
    style.Colors[ImGuiCol_TabActive]          = accent_color;
    style.Colors[ImGuiCol_TabUnfocused]       = ImVec4(accent_color.x * 0.2f, accent_color.y * 0.2f, accent_color.z * 0.2f, 0.97f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(accent_color.x * 0.5f, accent_color.y * 0.5f, accent_color.z * 0.5f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]      = ImVec4(accent_color.x * 0.6f, accent_color.y * 0.6f, accent_color.z * 0.6f, 1.00f);
    style.Colors[ImGuiCol_DockingPreview]     = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.7f);
    style.Colors[ImGuiCol_ResizeGrip]         = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.5f);
    style.Colors[ImGuiCol_ResizeGripHovered]  = ImVec4(accent_color.x, accent_color.y, accent_color.z, 0.8f);
    style.Colors[ImGuiCol_ResizeGripActive]   = accent_color;
}