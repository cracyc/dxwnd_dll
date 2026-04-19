#define _CRT_SECURE_NO_WARNINGS
#define INITGUID
//#define FULLHEXDUMP

#define _MODULE "dxwnd"

#include <windows.h>
#include <ddraw.h>
#include "dxwnd.h"
#include "dxhook.h"
#include "ddrawi.h"
#include "dxwcore.hpp"
#include "stdio.h" 
#include "hddraw.h"
#include "dxhelper.h"
#include "syslibs.h"
#include "d3d9.h"
#include "d3d9shader.h"
#include "d3d9blit.h"
#include "ddrender.h"

#define ENHANCEDDXWNDHANDLING
//#define RENDERDEBUG
#ifdef RENDERDEBUG
#define OutDebug(f, ...) OutTrace(f, __VA_ARGS__)
#else
#define OutDebug(f, ...)
#endif

extern Lock_Type pLockMethod(int);
extern Unlock4_Type pUnlockMethod(int);
extern HRESULT DDRawBlitToEmu(int , Blt_Type, LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

static BOOL d3d9_create_resouces();
static BOOL d3d9_set_states();
static BOOL d3d9_update_vertices(BOOL upscale_hack, BOOL stretch);

static D3D9RENDERER g_d3d9;
static DDRENDERER gddraw = { 0 };
DDRENDERER *g_ddraw = &gddraw;

BOOL d3d9_is_available()
{
    LPDIRECT3D9 d3d9 = NULL;
	HMODULE hmodule;

    if ((hmodule = (*pLoadLibraryA)("d3d9.dll"))){
        IDirect3D9* (WINAPI * d3d_create9)(UINT) =
            (IDirect3D9 * (WINAPI*)(UINT))(*pGetProcAddress)(g_d3d9.hmodule, "Direct3DCreate9");
        if (d3d_create9 && (d3d9 = d3d_create9(D3D_SDK_VERSION))) IDirect3D9_Release(d3d9);
    }
    return d3d9 != NULL;
}

BOOL d3d9_create()
{
	ApiName("d3d9_create");

	if (!d3d9_release()){
		OutErrorD3D("%s: d3d9_release ERROR @%d\n", ApiRef, __LINE__);
        return FALSE;
	}

    if (!g_d3d9.hmodule)
        g_d3d9.hmodule = LoadLibrary("d3d9.dll");

    if (g_d3d9.hmodule)
    {
        if (g_ddraw->nonexclusive)
        {
            int (WINAPI * d3d9_enable_shim)(BOOL) =
                (int (WINAPI*)(BOOL))GetProcAddress(g_d3d9.hmodule, "Direct3D9EnableMaximizedWindowedModeShim");

            if (d3d9_enable_shim)
                d3d9_enable_shim(TRUE);
        }

        IDirect3D9* (WINAPI * d3d_create9)(UINT) =
            (IDirect3D9 * (WINAPI*)(UINT))GetProcAddress(g_d3d9.hmodule, "Direct3DCreate9");

        if (d3d_create9 && (g_d3d9.instance = d3d_create9(D3D_SDK_VERSION)))
        {
            //g_d3d9.bits_per_pixel = g_ddraw->render.bpp ? g_ddraw->render.bpp : g_ddraw->mode.dmBitsPerPel;
			g_d3d9.bits_per_pixel = dxw.VirtualPixelFormat.dwRGBBitCount;

            memset(&g_d3d9.params, 0, sizeof(g_d3d9.params));

            g_d3d9.params.Windowed = g_ddraw->windowed;
            g_d3d9.params.SwapEffect = D3DSWAPEFFECT_DISCARD;
            //g_d3d9.params.hDeviceWindow = g_ddraw->hwnd;
            g_d3d9.params.hDeviceWindow = dxw.GethWnd();
            //g_d3d9.params.PresentationInterval = g_ddraw->vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
            g_d3d9.params.PresentationInterval = (dxw.dwFlags8 & FORCEVSYNC) ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
            //g_d3d9.params.BackBufferWidth = g_d3d9.params.Windowed ? 0 : g_ddraw->render.width;
            //g_d3d9.params.BackBufferHeight = g_d3d9.params.Windowed ? 0 : g_ddraw->render.height;
            g_d3d9.params.BackBufferWidth = dxw.Windowize ? 0 : dxw.GetScreenWidth();
            g_d3d9.params.BackBufferHeight = dxw.Windowize ? 0 : dxw.GetScreenHeight();
            g_d3d9.params.BackBufferFormat = dxw.ActualPixelFormat.dwRGBBitCount == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
            g_d3d9.params.BackBufferCount = 1;

            DWORD behavior_flags[] = {
                D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,
                D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,
                D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING,
                D3DCREATE_MULTITHREADED | D3DCREATE_MIXED_VERTEXPROCESSING,
                D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            };

            for (int i = 0; i < sizeof(behavior_flags) / sizeof(behavior_flags[0]); i++)
            {
                if (SUCCEEDED(
                    IDirect3D9_CreateDevice(
                        g_d3d9.instance,
                        D3DADAPTER_DEFAULT,
                        D3DDEVTYPE_HAL,
                        dxw.GethWnd(),
                        //behavior_flags[i] | (g_ddraw->fpupreserve ? D3DCREATE_FPU_PRESERVE : 0),
                        behavior_flags[i] | D3DCREATE_FPU_PRESERVE,
                        &g_d3d9.params,
						&g_d3d9.device))){
							OutDebugD3D("%s: pick behavior_flag=%#x\n", ApiRef, behavior_flags[i] | D3DCREATE_FPU_PRESERVE);
                    return g_d3d9.device && d3d9_create_resouces() && d3d9_set_states();
				}
            }
        }
    }
	OutDebug("%s: no suitable behaviorfound\n", ApiRef);
    return FALSE;
}

BOOL d3d9_on_device_lost()
{
    if (g_d3d9.device && IDirect3DDevice9_TestCooperativeLevel(g_d3d9.device) == D3DERR_DEVICENOTRESET){
        return d3d9_reset();
    }

    return FALSE;
}

BOOL d3d9_reset()
{
    //g_d3d9.params.Windowed = g_ddraw->windowed;
    g_d3d9.params.Windowed = dxw.Windowize;
    //g_d3d9.params.BackBufferWidth = g_d3d9.params.Windowed ? 0 : g_ddraw->render.width;
    //g_d3d9.params.BackBufferHeight = g_d3d9.params.Windowed ? 0 : g_ddraw->render.height;
    //g_d3d9.params.BackBufferFormat = g_d3d9.bits_per_pixel == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;
    g_d3d9.params.BackBufferWidth = dxw.Windowize ? 0 : dxw.GetScreenWidth();
    g_d3d9.params.BackBufferHeight = dxw.Windowize ? 0 : dxw.GetScreenHeight();
    g_d3d9.params.BackBufferFormat = dxw.ActualPixelFormat.dwRGBBitCount == 16 ? D3DFMT_R5G6B5 : D3DFMT_X8R8G8B8;

    if (g_d3d9.device && SUCCEEDED(IDirect3DDevice9_Reset(g_d3d9.device, &g_d3d9.params))){
        return d3d9_set_states();
    }

    return FALSE;
}

BOOL d3d9_release()
{
    if (g_d3d9.pixel_shader){
        IDirect3DPixelShader9_Release(g_d3d9.pixel_shader);
        g_d3d9.pixel_shader = NULL;
    }

    if (g_d3d9.pixel_shader_bilinear){
        IDirect3DPixelShader9_Release(g_d3d9.pixel_shader_bilinear);
        g_d3d9.pixel_shader_bilinear = NULL;
    }

    for (int i = 0; i < D3D9_TEXTURE_COUNT; i++){
        if (g_d3d9.surface_tex[i]) {
            IDirect3DTexture9_Release(g_d3d9.surface_tex[i]);
            g_d3d9.surface_tex[i] = NULL;
        }

        if (g_d3d9.palette_tex[i]){
            IDirect3DTexture9_Release(g_d3d9.palette_tex[i]);
            g_d3d9.palette_tex[i] = NULL;
        }
    }

    if (g_d3d9.vertex_buf){
        IDirect3DVertexBuffer9_Release(g_d3d9.vertex_buf);
        g_d3d9.vertex_buf = NULL;
    }

    if (g_d3d9.device){
        IDirect3DDevice9_Release(g_d3d9.device);
        g_d3d9.device = NULL;
    }

    if (g_d3d9.instance){
        IDirect3D9_Release(g_d3d9.instance);
        g_d3d9.instance = NULL;
    }

    return TRUE;
}

static BOOL d3d9_create_resouces()
{
	ApiName("d3d9_create_resouces");
    BOOL err = FALSE;
	HRESULT res;

    int width = dxw.GetScreenWidth();
    int height = dxw.GetScreenHeight();

    g_d3d9.tex_width =
        width <= 1024 ? 1024 : width <= 2048 ? 2048 : width <= 4096 ? 4096 : width;

    g_d3d9.tex_height =
        height <= g_d3d9.tex_width ? g_d3d9.tex_width : height <= 2048 ? 2048 : height <= 4096 ? 4096 : height;

    g_d3d9.tex_width = g_d3d9.tex_width > g_d3d9.tex_height ? g_d3d9.tex_width : g_d3d9.tex_height;

    g_d3d9.scale_w = (float)width / g_d3d9.tex_width;
    g_d3d9.scale_h = (float)height / g_d3d9.tex_height;

    res = IDirect3DDevice9_CreateVertexBuffer(
        g_d3d9.device,
        sizeof(CUSTOMVERTEX) * 4, 0,
        D3DFVF_XYZRHW | D3DFVF_TEX1,
        D3DPOOL_MANAGED,
        &g_d3d9.vertex_buf,
        NULL);
	if(res) {
		OutTraceD3D("%s: IDirect3DDevice9_CreateVertexBuffer ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}

    res = d3d9_update_vertices(InterlockedExchangeAdd(&g_ddraw->upscale_hack_active, 0), TRUE);
	if(!res) {
		OutTraceD3D("%s: d3d9_update_vertices ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}

    for (int i = 0; i < D3D9_TEXTURE_COUNT; i++)
    {
		res = IDirect3DDevice9_CreateTexture(
                g_d3d9.device,
                g_d3d9.tex_width,
                g_d3d9.tex_height,
                1,
                0,
                g_ddraw->bpp == 16 ? D3DFMT_R5G6B5 : g_ddraw->bpp == 32 ? D3DFMT_X8R8G8B8 : D3DFMT_L8,
                D3DPOOL_MANAGED,
                &g_d3d9.surface_tex[i],
                0);
		if(res || !g_d3d9.surface_tex[i]){
			OutErrorD3D("%s: IDirect3DDevice9_CreateTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
			err = TRUE;
		}
        //err = err || !g_d3d9.surface_tex[i];

        if (g_ddraw->bpp == 8)
        {
			res = IDirect3DDevice9_CreateTexture(
				g_d3d9.device,
				256,
				256,
				1,
				0,
				D3DFMT_X8R8G8B8,
				D3DPOOL_MANAGED,
				&g_d3d9.palette_tex[i],
				0);
			if(res || !g_d3d9.palette_tex[i]){
				OutErrorD3D("%s: IDirect3DDevice9_CreateTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				err = TRUE;
			}
        }
    }

    if (g_ddraw->bpp == 8)
    {
        res = IDirect3DDevice9_CreatePixelShader(g_d3d9.device, (DWORD*)D3D9_PALETTE_SHADER, &g_d3d9.pixel_shader);
		if(res) {
			OutErrorD3D("%s: IDirect3DDevice9_CreatePixelShader ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
			err = TRUE;
		}

        res = IDirect3DDevice9_CreatePixelShader(
            g_d3d9.device, 
            (DWORD*)D3D9_PALETTE_SHADER_BILINEAR, 
            &g_d3d9.pixel_shader_bilinear);
		if(res) {
			OutErrorD3D("%s: IDirect3DDevice9_CreatePixelShader ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
			//err = TRUE;
		}
    }

	OutTraceD3D("%s: g_ddraw->bpp=%d\n", ApiRef, g_ddraw->bpp);
    return g_d3d9.vertex_buf && (g_d3d9.pixel_shader || g_ddraw->bpp == 16 || g_ddraw->bpp == 32) && !err;
}

static BOOL d3d9_set_states()
{
	ApiName("d3d9_set_states");
    BOOL err = FALSE;
	HRESULT res;

    res = IDirect3DDevice9_SetFVF(g_d3d9.device, D3DFVF_XYZRHW | D3DFVF_TEX1);
	if(res) {
		OutErrorD3D("%s: IDirect3DDevice9_SetFVF ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}
    res = IDirect3DDevice9_SetStreamSource(g_d3d9.device, 0, g_d3d9.vertex_buf, 0, sizeof(CUSTOMVERTEX));
	if(res) {
		OutErrorD3D("%s: IDirect3DDevice9_SetStreamSource ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}
    res = IDirect3DDevice9_SetTexture(g_d3d9.device, 0, (IDirect3DBaseTexture9*)g_d3d9.surface_tex[0]);
	if(res) {
		OutErrorD3D("%s: IDirect3DDevice9_SetTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}

    if (g_ddraw->bpp == 8)
    {
        res = IDirect3DDevice9_SetTexture(g_d3d9.device, 1, (IDirect3DBaseTexture9*)g_d3d9.palette_tex[0]);
		if(res) {
			OutErrorD3D("%s: IDirect3DDevice9_SetTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
			err = TRUE;
		}
        
        //BOOL bilinear =
        //    g_ddraw->d3d9linear &&
        //    g_d3d9.pixel_shader_bilinear &&
        //    (g_ddraw->viewport.width != g_ddraw->width || g_ddraw->viewport.height != g_ddraw->height);
		//BOOL bilinear = FALSE;
		//BOOL bilinear = TRUE;	
		BOOL bilinear = (dxw.dwFlags5 & BILINEARFILTER);

        res = IDirect3DDevice9_SetPixelShader(g_d3d9.device, 
			bilinear ? g_d3d9.pixel_shader_bilinear : g_d3d9.pixel_shader);
		if(res) {
			OutErrorD3D("%s: IDirect3DDevice9_SetPixelShader ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
			err = TRUE;
		}

        if (bilinear)
        {
            float texture_size[4] = { (float)g_d3d9.tex_width, (float)g_d3d9.tex_height, 0, 0 };
            res = IDirect3DDevice9_SetPixelShaderConstantF(g_d3d9.device, 0, texture_size, 1);
			if(res) {
				OutErrorD3D("%s: IDirect3DDevice9_SetPixelShaderConstantF ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				err = TRUE;
			}
        }
    }
    else
    {
        if (g_ddraw->d3d9linear)
        {
            IDirect3DDevice9_SetSamplerState(g_d3d9.device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            IDirect3DDevice9_SetSamplerState(g_d3d9.device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        }
    }

    D3DVIEWPORT9 view_data = {
        g_ddraw->viewport.x,
        g_ddraw->viewport.y,
        g_ddraw->viewport.width,
        g_ddraw->viewport.height,
        0.0f,
        1.0f };

    res = IDirect3DDevice9_SetViewport(g_d3d9.device, &view_data);
	if(res) {
		OutErrorD3D("%s: IDirect3DDevice9_SetViewport ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
		err = TRUE;
	}
	return !err;
}

static BOOL d3d9_update_vertices(BOOL upscale_hack, BOOL stretch)
{
    float vp_x = stretch ? (float)g_ddraw->viewport.x : 0.0f;
    float vp_y = stretch ? (float)g_ddraw->viewport.y : 0.0f;

    float vp_w = stretch ? (float)(g_ddraw->viewport.width + g_ddraw->viewport.x) : (float)g_ddraw->width;
    float vp_h = stretch ? (float)(g_ddraw->viewport.height + g_ddraw->viewport.y) : (float)g_ddraw->height;

    //float s_h = upscale_hack ? g_d3d9.scale_h * ((float)g_ddraw->upscale_hack_height / g_ddraw->height) : g_d3d9.scale_h;
    //float s_w = upscale_hack ? g_d3d9.scale_w * ((float)g_ddraw->upscale_hack_width / g_ddraw->width) : g_d3d9.scale_w;
    float s_h = g_d3d9.scale_h;
    float s_w = g_d3d9.scale_w;

    CUSTOMVERTEX vertices[] =
    {
        { vp_x - 0.5f, vp_h - 0.5f, 0.0f, 1.0f, 0.0f, s_h },
        { vp_x - 0.5f, vp_y - 0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
        { vp_w - 0.5f, vp_h - 0.5f, 0.0f, 1.0f, s_w,  s_h },
        { vp_w - 0.5f, vp_y - 0.5f, 0.0f, 1.0f, s_w,  0.0f }
    };

    void* data;
    if (g_d3d9.vertex_buf && SUCCEEDED(IDirect3DVertexBuffer9_Lock(g_d3d9.vertex_buf, 0, 0, (void**)&data, 0)))
    {
        memcpy(data, vertices, sizeof(vertices));

        IDirect3DVertexBuffer9_Unlock(g_d3d9.vertex_buf);
        return TRUE;
    }

    return FALSE;
}

static void D3D9FrameBlit()
{
	ApiName("D3D9FrameBlit");
	HRESULT res;
	static int clear_count = 20;

    //Sleep(500);

    static int tex_index = 0, pal_index = 0;

    //if (InterlockedExchange(&g_ddraw->clear_screen, FALSE))
    //    clear_count = 10;

    //if (g_ddraw->primary && 
    //    g_ddraw->primary->bpp == g_ddraw->bpp &&
    //    (g_ddraw->bpp == 16 || g_ddraw->bpp == 32 || g_ddraw->primary->palette))
	if(1)
    {
        //if (g_ddraw->vhack)
		if(1)
        {
            //if (util_detect_low_res_screen())
            if (0)
            {
               if (!InterlockedExchange(&g_ddraw->upscale_hack_active, TRUE))
                    d3d9_update_vertices(TRUE, TRUE);
            }
            else
            {
                if (InterlockedExchange(&g_ddraw->upscale_hack_active, FALSE))
                    d3d9_update_vertices(FALSE, TRUE);
            }
        }

       D3DLOCKED_RECT lock_rc;

        //if (InterlockedExchange(&g_ddraw->surface_updated, FALSE) || g_ddraw->render.minfps == -2)
        if (InterlockedExchange(&g_ddraw->surface_updated, FALSE)){
			if (++tex_index >= D3D9_TEXTURE_COUNT) tex_index = 0;
            RECT rc = { 0, 0, g_ddraw->width, g_ddraw->height };

			res = IDirect3DDevice9_SetTexture(g_d3d9.device, 0, (IDirect3DBaseTexture9*)g_d3d9.surface_tex[tex_index]);
			if(res) {
				OutErrorD3D("%s: IDirect3DDevice9_SetTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				return;
			}
			res = IDirect3DTexture9_LockRect(g_d3d9.surface_tex[tex_index], 0, &lock_rc, &rc, 0);
			if(res) {
				OutErrorD3D("%s: IDirect3DDevice9_SetTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				return;
			}

			DDSURFACEDESC2 ddsd;
			memset(&ddsd,0,sizeof(DDSURFACEDESC2));
			ddsd.dwSize = Set_dwSize_From_Surface();
			ddsd.dwFlags = DDSD_LPSURFACE | DDSD_PITCH;
			if(res=(*pLockMethod(dxw.BlitterDXVersion))(dxw.lpBlitterSurface, 0, (LPDDSURFACEDESC)&ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_READONLY, 0)){	
				OutErrorD3D("%s: Lock ERROR res=%#x(%s) @%d\n", ApiRef, res, ExplainDDError(res), __LINE__);
				res=(*pUnlockMethod(dxw.BlitterDXVersion))(dxw.lpBlitterSurface, NULL);
				// unlock to avoid deadlocks
				IDirect3DTexture9_UnlockRect(g_d3d9.surface_tex[tex_index], 0);
				return;
			}
			g_ddraw->l_pitch = ddsd.lPitch;
			g_ddraw->width = ddsd.dwWidth;
			g_ddraw->height = ddsd.dwHeight;
			g_ddraw->pBits = (char *)ddsd.lpSurface;
			g_ddraw->bpp = ddsd.ddpfPixelFormat.dwRGBBitCount;
			OutDebugD3D("%s: l_pitch=%d bpp=%d size=%dx%d\n", 
				ApiRef,
				g_ddraw->l_pitch, 
				g_ddraw->bpp,
				g_ddraw->width, 
				g_ddraw->height);

            //unsigned char* src = (unsigned char*)g_ddraw->primary->surface;
            unsigned char* src = (unsigned char*)g_ddraw->pBits;
            unsigned char* dst = (unsigned char*)lock_rc.pBits;
            for (int i = 0; i < g_ddraw->height; i++) {
                memcpy(dst, src, g_ddraw->l_pitch);
                src += g_ddraw->l_pitch;
                dst += lock_rc.Pitch;
            }
// unlock
			res=(*pUnlockMethod(dxw.BlitterDXVersion))(dxw.lpBlitterSurface, NULL);

			IDirect3DTexture9_UnlockRect(g_d3d9.surface_tex[tex_index], 0);
			if(res) {
				OutErrorD3D("%s: IDirect3DTexture9_UnlockRect ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				//continue;
			}
        }

        if (g_ddraw->bpp == 8 &&
            //(InterlockedExchange(&g_ddraw->palette_updated, FALSE) || g_ddraw->render.minfps == -2))
            //InterlockedExchange(&g_ddraw->palette_updated, FALSE))
			TRUE)
        {
            if (++pal_index >= D3D9_TEXTURE_COUNT)
                pal_index = 0;

            RECT rc = { 0,0,256,1 };

            //if (SUCCEEDED(IDirect3DDevice9_SetTexture(g_d3d9.device, 1, (IDirect3DBaseTexture9*)g_d3d9.palette_tex[pal_index])) &&
            //    SUCCEEDED(IDirect3DTexture9_LockRect(g_d3d9.palette_tex[pal_index], 0, &lock_rc, &rc, 0)))
			res = IDirect3DDevice9_SetTexture(g_d3d9.device, 1, (IDirect3DBaseTexture9*)g_d3d9.palette_tex[pal_index]);
			if(res) {
				OutErrorD3D("%s: IDirect3DDevice9_SetTexture ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				//continue;
			}
			res = IDirect3DTexture9_LockRect(g_d3d9.palette_tex[pal_index], 0, &lock_rc, &rc, 0);
			if(res) {
				OutErrorD3D("%s: IDirect3DTexture9_LockRect ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
				//continue;
			}
			if(1)
			{
               //memcpy(lock_rc.pBits, g_ddraw->primary->palette->data_rgb, 256 * sizeof(int));
				extern DWORD PaletteEntries[256];
				PALETTEENTRY Palette[256];		
				for(int i=0; i<256; i++){
					DWORD px = PaletteEntries[i];
					LPPALETTEENTRY pe = &Palette[i];
					pe->peRed = (px >> 0) & 0xFF;
					pe->peGreen = (px >> 8) & 0xFF;
					pe->peBlue = (px >> 16) & 0xFF;
					pe->peFlags = 0;
				}
                memcpy(lock_rc.pBits, Palette, 256 * sizeof(PALETTEENTRY));
                res = IDirect3DTexture9_UnlockRect(g_d3d9.palette_tex[pal_index], 0);
				if(res) {
					OutErrorD3D("%s: IDirect3DTexture9_LockRect ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
					//continue;
				}
            }
        }

      //if (g_ddraw->fixchilds)
        //{
        //    g_ddraw->child_window_exists = FALSE;
        //    EnumChildWindows(g_ddraw->hwnd, util_enum_child_proc, (LPARAM)g_ddraw->primary);

            //if (g_ddraw->render.width != g_ddraw->width || g_ddraw->render.height != g_ddraw->height)
            //{
            //    if (g_ddraw->child_window_exists)
            //    {
            //        IDirect3DDevice9_Clear(g_d3d9.device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

            //        if (!needs_update && d3d9_update_vertices(FALSE, FALSE))
            //            needs_update = TRUE;
            //    }
            //    else if (needs_update)
            //    {
            //        if (d3d9_update_vertices(FALSE, TRUE))
            //            needs_update = FALSE;
            //    }
            //}
        //}
    }

    if (clear_count > 0){
        clear_count--;
        res = IDirect3DDevice9_Clear(g_d3d9.device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		if(res) OutErrorD3D("%s: IDirect3DDevice9_Clear ERROR res=%#x @%d\n", ApiRef, res, __LINE__);
   }

    res=IDirect3DDevice9_BeginScene(g_d3d9.device);
	if(res) OutErrorD3D("%s: IDirect3DDevice9_BeginScene ERROR err=%#x @%d\n", ApiRef, res, __LINE__);
    res=IDirect3DDevice9_DrawPrimitive(g_d3d9.device, D3DPT_TRIANGLESTRIP, 0, 2);
	if(res) OutErrorD3D("%s: IDirect3DDevice9_DrawPrimitive ERROR err=%#x @%d\n", ApiRef, res, __LINE__);
    res=IDirect3DDevice9_EndScene(g_d3d9.device);
	if(res) OutErrorD3D("%s: Direct3DDevice9_EndScene ERROR err=%#x @%d\n", ApiRef, res, __LINE__);

    //if (g_ddraw->bnet_active)
    //{
    //    IDirect3DDevice9_Clear(g_d3d9.device, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    //}

    res = IDirect3DDevice9_Present(g_d3d9.device, NULL, NULL, NULL, NULL);
    if (res) {
		OutErrorD3D("%s: IDirect3DDevice9_Present ERROR err=%#x @%d\n", ApiRef, res, __LINE__);
        //DWORD_PTR result;
        //SendMessageTimeout(g_ddraw->hwnd, WM_D3D9DEVICELOST, 0, 0, 0, 1000, &result);
    }
}

static DWORD WINAPI D3D9AsyncThread(LPVOID lpParameter)
{
	while(TRUE){
		extern void LimitFrameHz(DWORD);  
		LimitFrameHz(60);

		if (!dxw.lpBlitterSurface) continue;

		// Request ownership of the critical section.
		EnterCriticalSection(&dxw.CriticalSection); 
		D3D9FrameBlit();
		// Release ownership of the critical section.
		LeaveCriticalSection(&dxw.CriticalSection);
	}
}


HRESULT D3D9AsyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res;

	if(!dxw.bAsybcBlitStarted) {
		if (!InitializeCriticalSectionAndSpinCount(&dxw.CriticalSection, 0x00000400) ) {
			OutErrorD3D("%s: ERROR in InitializeCriticalSectionAndSpinCount err=%d\n", api, GetLastError());
			return -1;
		}
		memset(&g_d3d9, 0, sizeof(g_d3d9));
		g_d3d9.hmodule = NULL;
		g_ddraw->bpp = dxw.VirtualPixelFormat.dwRGBBitCount;
		g_ddraw->windowed = dxw.Windowize;
		g_ddraw->hwnd = dxw.GethWnd();
		g_ddraw->d3d9linear = TRUE; // ???
		g_ddraw->viewport.x = 0;
		g_ddraw->viewport.y = 0;
		g_ddraw->viewport.width = dxw.iSizX;
		g_ddraw->viewport.height = dxw.iSizY;
		g_ddraw->width = dxw.GetScreenWidth();
		g_ddraw->height = dxw.GetScreenHeight();
		g_ddraw->palette_updated = TRUE; // ???
		dxw.bAsybcBlitStarted = TRUE;
		if(!d3d9_create()) {
			OutErrorD3D("%s: d3d9_create ERROR @%d\n", api, __LINE__);
			return -1;
		}
		CreateThread(NULL, 0, D3D9AsyncThread, (LPVOID)lpdds, 0, NULL);
	}

	dxw.lpBlitterSurface = lpdds;
	dxw.BlitterDXVersion = dxversion;

	EnterCriticalSection(&dxw.CriticalSection);
#ifdef ENHANCEDDXWNDHANDLING
	lpdds->SetClipper(NULL);
	res = DDRawBlitToEmu(dxversion, pBlt, lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
#else
	res=(*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
#endif

	if(res == DD_OK){
		InterlockedExchange(&g_ddraw->surface_updated, TRUE);
		LeaveCriticalSection(&dxw.CriticalSection);
		SwitchToThread();
	}
	else {
		LeaveCriticalSection(&dxw.CriticalSection);
	}
	return DD_OK;
}

HRESULT D3D9SyncBlitter(int dxversion, Blt_Type pBlt, char *api, LPDIRECTDRAWSURFACE lpdds, LPRECT lpdestrect,
	LPDIRECTDRAWSURFACE s, LPRECT lpsrcrect, DWORD dwflags, LPDDBLTFX lpddbltfx, BOOL isFlipping)
{
	HRESULT res;
	OutDebug("D3D9SyncBlitter\n");

	if(!dxw.bAsybcBlitStarted) {
		memset(&g_d3d9, 0, sizeof(g_d3d9));
		g_d3d9.hmodule = NULL;
		g_ddraw->bpp = dxw.VirtualPixelFormat.dwRGBBitCount;
		g_ddraw->windowed = dxw.Windowize;
		g_ddraw->hwnd = dxw.GethWnd();
		g_ddraw->d3d9linear = TRUE; // ???
		g_ddraw->viewport.x = 0;
		g_ddraw->viewport.y = 0;
		g_ddraw->viewport.width = dxw.iSizX;
		g_ddraw->viewport.height = dxw.iSizY;
		g_ddraw->width = dxw.GetScreenWidth();
		g_ddraw->height = dxw.GetScreenHeight();
		g_ddraw->palette_updated = TRUE; // ???
		dxw.bAsybcBlitStarted = TRUE;
		if(!d3d9_create()) {
			OutErrorD3D("%s: d3d9_create ERROR @%d\n", ApiRef, __LINE__);
			return -1;
		}
	}

	dxw.lpBlitterSurface = lpdds;
	dxw.BlitterDXVersion = dxversion;

	res=(*pBlt)(lpdds, lpdestrect, s, lpsrcrect, dwflags, lpddbltfx);
	if(res == DD_OK){
		InterlockedExchange(&g_ddraw->surface_updated, TRUE);
		D3D9FrameBlit();
	}
	return DD_OK;
}