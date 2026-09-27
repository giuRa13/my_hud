#ifndef APP_LAYER_H
#define APP_LAYER_H

#include <windows.h>
#include <d3d9.h>
#include <exe/gui_layer.h>

class App
{
public:
    App();
    ~App() = default;

    bool init();
    void run();
    void shutdown();

    // static pointer for the WndProc hook callback
    static App* instance;

    LRESULT handle_Message(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd = nullptr;
    WNDCLASSEXW wc = {};
    LPDIRECT3D9 pD3D = nullptr;
    LPDIRECT3DDEVICE9 pd3dDevice = nullptr;
    D3DPRESENT_PARAMETERS d3dpp = {};

    Gui_Layer gui_layer;
    bool running = true;

    bool create_DeviceD3D();
    void cleanup_DeviceD3D();
    void reset_Device();

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif