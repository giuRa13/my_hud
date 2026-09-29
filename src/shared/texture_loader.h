#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <imgui.h>
#include <stb_image/stb_image.h>

#ifdef IS_CONTROL_PANEL
    #include <d3d9.h>
#elif defined(IS_OVERLAY_DLL)
    #include <d3d11.h>
#endif

namespace widgets
{

// set once by each target during its own init (Gui_Layer::init / hkPresent's first-frame setup)
#ifdef IS_CONTROL_PANEL
    inline LPDIRECT3DDEVICE9 g_texture_device = nullptr;
#elif defined(IS_OVERLAY_DLL)
    inline ID3D11Device*        g_texture_device  = nullptr;
    inline ID3D11DeviceContext* g_texture_context = nullptr;
#endif

    inline void platform_free_texture(ImTextureID t)
    {
        if (!t) return;
#ifdef IS_CONTROL_PANEL
        ((LPDIRECT3DTEXTURE9)(intptr_t)t)->Release();
#elif defined(IS_OVERLAY_DLL)
        ((ID3D11ShaderResourceView*)(intptr_t)t)->Release();
#endif
    }

    inline bool platform_load_texture(const char* path, ImTextureID& out, int& w, int& h)
    {
#ifdef IS_CONTROL_PANEL
        if (!g_texture_device) return false;
        int ch = 0;
        unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
        if (!data) return false;

        LPDIRECT3DTEXTURE9 tex = nullptr;
        // D3DX_DEFAULT mip levels = full chain; D3DPOOL_MANAGED required for mipmapped textures loaded this way
        if (FAILED(g_texture_device->CreateTexture(w, h, 0, D3DUSAGE_AUTOGENMIPMAP,
                                                     D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
        {
            // fallback: some drivers don't support AUTOGENMIPMAP for this format
            if (FAILED(g_texture_device->CreateTexture(w, h, 0, 0,
                                                         D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr)))
            {
                stbi_image_free(data);
                return false;
            }
        }

        D3DLOCKED_RECT r;
        if (SUCCEEDED(tex->LockRect(0, &r, nullptr, 0)))
        {
            for (int y = 0; y < h; ++y)
            {
                unsigned char* dst = (unsigned char*)r.pBits + y * r.Pitch;
                const unsigned char* src = data + y * w * 4;
                for (int x = 0; x < w; ++x, dst += 4, src += 4)
                {
                    dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = src[3]; // RGBA -> BGRA
                }
            }
            tex->UnlockRect(0);
        }
        stbi_image_free(data);
        tex->GenerateMipSubLevels(); // needed if AUTOGENMIPMAP path wasn't honored by the driver

        out = (ImTextureID)(intptr_t)tex;
        return true;

#elif defined(IS_OVERLAY_DLL)
        if (!g_texture_device || !g_texture_context) return false;
        int ch = 0;
        unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
        if (!data) return false;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w; desc.Height = h;
        desc.MipLevels = 0;                                                     // 0 = let D3D generate a full mip chain
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;                                       // must be DEFAULT (not IMMUTABLE) to allow GenerateMips
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // RENDER_TARGET required for GenerateMips
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

        ID3D11Texture2D* tex = nullptr;
        // create WITHOUT initial data (mip 0 only gets filled next, via UpdateSubresource)
        bool ok = SUCCEEDED(g_texture_device->CreateTexture2D(&desc, nullptr, &tex));

        ID3D11ShaderResourceView* srv = nullptr;
        if (ok)
        {
            g_texture_context->UpdateSubresource(tex, 0, nullptr, data, w * 4, 0); // fill mip 0
            ok = SUCCEEDED(g_texture_device->CreateShaderResourceView(tex, nullptr, &srv));
            if (ok) g_texture_context->GenerateMips(srv);  // build the rest of the chain from mip 0
        }
        if (tex) tex->Release();
        stbi_image_free(data);

        if (!ok) return false;
        out = (ImTextureID)(intptr_t)srv;
        return true;
#else
        return false; // neither target macro defined — shouldn't happen
#endif
    }
    
}

#endif