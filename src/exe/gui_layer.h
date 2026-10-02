#ifndef GUI_LAYER_H
#define GUI_LAYER_H

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx9.h>
#include <d3d9.h>
#include <shared/config.h> 

class Gui_Layer
{
public:
	Gui_Layer() = default;
    ~Gui_Layer() = default;

	void init(HWND hwnd, LPDIRECT3DDEVICE9 g_pd3dDevice);
	void begin();
	void end();
    void shutdown();
    void draw_widgets();
    void set_theme();
    void apply_accent_color();

    bool& get_vsync_modified() { return vsync_changed; }
    bool& get_show_demo() { return show_demo; }
    float get_opacity() const { return config.opacity; }
    bool get_vsync() const { return config.enable_vsync; }
    ImVec4 get_accent_color() const { return ImVec4(config.accent_r, config.accent_g, config.accent_b, config.accent_a); }

private:
    config::AppConfig config;
    ImVec4 accent_color;
    bool vsync_changed = false;
    bool show_demo = false;

    bool show_pedals_settings = false; 
    bool show_delta_settings = false;
    bool show_gear_settings = false;
    bool show_wheel_settings = false;
    bool show_lap_history_settings = false;
};

#endif