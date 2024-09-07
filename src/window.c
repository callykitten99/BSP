/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Facilitates interaction with the GUI window.
*******************************************************************************/

#include "camera.h"
#include "draw.h"
#include "select.h"
#include "bsp.h"
#include "window.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdio.h>

#define CLASS_NAME "ENOBY_BSP"
#define APP_NAME   "Enoby BSP Viewer"



LRESULT CALLBACK WinMsgProc(HWND,UINT,WPARAM,LPARAM);


extern pools g_pool;
extern bsp   g_bsp;
extern unsigned short
    g_bsp_l, g_bsp_r, g_pivot_l, g_pivot_r, g_pivot,
    g_poly_clip, g_poly_inter, g_vert_012[3];

extern DRAW_MODE g_draw_mode;
extern float g_ray[3];

LARGE_INTEGER f;
LARGE_INTEGER t0, t1;

//UINT_PTR     g_timer;
HWND         g_wnd;
unsigned int g_winw, g_winh;

bool g_lapse = false,
     g_stall = false,
     g_scheduleSelect = false;



bool
window_class_init(void)
{
    WNDCLASSEX wc =
    {
        sizeof(wc), /* cbSize */
        CS_VREDRAW | CS_HREDRAW | CS_OWNDC, /* style */
        WinMsgProc,
        0, 0, /* clsExtra, wndExtra */
        GetModuleHandle(NULL),
        NULL, /* Icon */
        NULL, /* Cursor */
        NULL, /* Brush. NULL means application must paint background */
        NULL, /* Menu, if we provide one in the resource file */
        CLASS_NAME, /* Class atom name */
        NULL, /* Small Icon */
    };

    return RegisterClassEx(&wc);
}



void
centred_window_rect(unsigned int w,
                    unsigned int h,
                    RECT *out)
{
    long scrw, scrh, ew, eh;
    RECT rct = {0,0,w,h};

    scrw = GetSystemMetrics(SM_CXSCREEN);
    scrh = GetSystemMetrics(SM_CYSCREEN);
    AdjustWindowRect(&rct, WS_OVERLAPPEDWINDOW, FALSE);
    ew = rct.right - rct.left;
    eh = rct.bottom - rct.top;
    out->left = (scrw>>1) - (ew>>1);
    out->top  = (scrh>>1) - (eh>>1);
    out->right = out->left + ew;
    out->bottom = out->top + eh;
}



bool
window_init_default(void)
{
    return window_init(512, 512);
}



bool
window_init(unsigned int w,
            unsigned int h)
{
    RECT rct;

    /* Update globals */
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    t1 = t0;
    g_winw = w;
    g_winh = h;

    if (!window_class_init())
    {
        fprintf(stderr, "Failed to create window class.\n");
        return false;
    }

    centred_window_rect(w,h,&rct);

    g_wnd = CreateWindowA(
        CLASS_NAME, APP_NAME,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        rct.left, rct.top,
        rct.right - rct.left,
        rct.bottom - rct.top,
        NULL, /* Parent window */
        NULL, /* No menu yet */
        GetModuleHandle(NULL),
        NULL /* Param */);

    if (!g_wnd)
        fprintf(stderr, "Failed to create window.\n");
    return g_wnd != NULL;
}



float
window_get_aspect_ratio(void)
{
    return (float)((double)g_winw / (double)g_winh);
}



bool
window_loop(void)
{
    MSG msg = {0};
    float delta;
    
    /* Get messages */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) return false;
        
        TranslateMessage(&msg);
        DispatchMessage (&msg);
    }

    /* Get time elapsed */
    QueryPerformanceCounter(&t1);
    delta = (float)((long double)(t1.QuadPart - t0.QuadPart) /
                    (long double)(f.QuadPart));
    
    if (delta >= 0.016666667f)
    {
        t0 = t1;

        /* Update camera */
        camera_update(delta);

        if (g_scheduleSelect)
        {
            g_scheduleSelect = false;
            select_begin(&g_pool);
        }
        
        draw(DRAW_MODE_UNSPECIFIED);
    }
    else
    {
        if (delta < 0.01f)
            Sleep(5);
        else
            Sleep(0);
    }

    return true;
}



void pivot_next(void)
{
    g_pivot_l = (g_pivot_r+1) % g_pool.n_faces;
    
    g_pivot_r = g_pivot_l;
    while (g_pivot_r < g_pool.n_faces-1 &&
           !memcmp(&g_pool.planes[g_pivot_r],
                   &g_pool.planes[g_pivot_l],
                   sizeof(plane)))
    {
        ++g_pivot_r;
    }
}

void pivot_prev(void)
{
    g_pivot_r = (g_pivot_l+g_pool.n_faces-1) % g_pool.n_faces;
    
    g_pivot_l = g_pivot_r;
    while (g_pivot_l > 0 &&
           !memcmp(&g_pool.planes[g_pivot_l],
                   &g_pool.planes[g_pivot_r],
                   sizeof(plane)))
    {
        --g_pivot_l;
    }
}



LRESULT CALLBACK
WinMsgProc(HWND win,
           UINT msg,
           WPARAM wp,
           LPARAM lp)
{
    switch (msg)
    {
#if 0
    case WM_TIMER:
        if (wp == 0)
        {
            g_lapse = false;
            //RedrawWindow(g_wnd, NULL, NULL, RDW_INTERNALPAINT);
            InvalidateRect(g_wnd, NULL, FALSE);
        }
        return 0;
#endif

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_CHAR:
        switch (LOWORD(wp))
        {
        case ']':
            if (!(lp & (1<<31)))
                pivot_next();
            return 0;
        case '[':
            if (!(lp & (1<<31)))
                pivot_prev();
            return 0;

        }
        break;

    case WM_KEYDOWN:
        switch (LOWORD(wp))
        {
        case '1':
            g_draw_mode = DRAW_MODE_LR;
            return 0;
        
        case '2':
            g_draw_mode = DRAW_MODE_REL;
            return 0;
        
        case '3':
            g_draw_mode = DRAW_MODE_CLIP;
            return 0;
        
        case '4':
            g_draw_mode = DRAW_MODE_BSP;
            return 0;
            
        case 'A':
            camera_set(CAM_LEFT);
            return 0;
        
        case 'D':
            camera_set(CAM_RIGHT);
            return 0;

        case 'W':
            camera_set(CAM_FWD);
            return 0;

        case 'S':
            camera_set(CAM_BACK);
            return 0;

        case 'Q':
            camera_set(CAM_UP);
            return 0;

        case 'E':
            camera_set(CAM_DOWN);
            return 0;
        
        case 'T':
            g_ray[2] += 0.1f;
            return 0;
        case 'G':
            g_ray[2] -= 0.1f;
            return 0;
        case 'F':
            g_ray[0] -= 0.1f;
            return 0;
        case 'H':
            g_ray[0] += 0.1f;
            return 0;
        case 'V':
            g_ray[1] -= 0.1f;
            return 0;
        case 'B':
            g_ray[1] += 0.1f;
            return 0;
        
        case 'P':
            if (g_bsp.d)
            {
                g_pivot_l = g_bsp.d[0].pl;
                g_pivot_r = g_bsp.d[0].pr;
                g_pivot = 0;
                g_bsp_l = 0;
                g_bsp_r = -1;
            }
            return 0;
        
        case 'I':
            if (g_bsp.d && g_pivot < g_bsp.occ)
            {
                bsp_ind next = g_bsp.d[g_pivot].p;
                if (next < g_bsp.occ)
                {
                    g_pivot = next;
                    g_pivot_l = g_bsp.d[next].pl;
                    g_pivot_r = g_bsp.d[next].pr;
                    return 0;
                }
                else
                {
                    g_pivot_l = g_bsp.d[0].pl;
                    g_pivot_r = g_bsp.d[0].pr;
                    g_pivot = 0;
                    g_bsp_l = 0;
                    g_bsp_r = -1;
                    return 0;
                }
            }
            g_pivot = -1;
            return 0;
        
        case 'J':
            if (g_bsp.d && g_pivot < g_bsp.occ)
            {
                bsp_ind next = g_bsp.d[g_pivot].l;
                if (next < g_bsp.occ)
                {
                    g_pivot = next;
                    g_pivot_l = g_bsp.d[next].pl;
                    g_pivot_r = g_bsp.d[next].pr;
                    return 0;
                }
                return 0;
            }
            g_pivot = -1;
            return 0;
        
        case 'K':
            if (g_bsp.d && g_pivot < g_bsp.occ)
            {
                bsp_ind next = g_bsp.d[g_pivot].r;
                if (next < g_bsp.occ)
                {
                    g_pivot = next;
                    g_pivot_l = g_bsp.d[next].pl;
                    g_pivot_r = g_bsp.d[next].pr;
                    return 0;
                }
                return 0;
            }
            g_pivot = -1;
            return 0;
        
        case VK_OEM_MINUS:
            camera_set_far(g_cam.farp - 1.f);
            return 0;
        
        case VK_OEM_PLUS:
            camera_set_far(g_cam.farp + 1.f);
            return 0;

        case VK_LEFT:
            camera_set(CAM_TURN_LEFT);
            return 0;

        case VK_RIGHT:
            camera_set(CAM_TURN_RIGHT);
            return 0;

        case VK_UP:
            camera_set(CAM_TURN_UP);
            return 0;

        case VK_DOWN:
            camera_set(CAM_TURN_DOWN);
            return 0;

        case VK_RETURN:
            g_stall = false;
            return 0;

        case VK_SPACE:
            if (!g_stall)
                g_scheduleSelect = true;            
            return 0;
        
        case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;

        case 'R':
            camera_reset();
            return 0;

        case 'M':
            float mat[16];
            camera_get_matrix(mat);

            fprintf(stderr, "[% 2.4f, % 2.4f, % 2.4f, % 8.4f]\n"
                            "[% 2.4f, % 2.4f, % 2.4f, % 8.4f]\n"
                            "[% 2.4f, % 2.4f, % 2.4f, % 8.4f]\n"
                            "[% 2.4f, % 2.4f, % 2.4f, % 8.4f]\n",
                            mat[0], mat[1], mat[2], mat[3],
                            mat[4], mat[5], mat[6], mat[7],
                            mat[8], mat[9], mat[10],mat[11],
                            mat[12],mat[13],mat[14],mat[15]);
            return 0;
        }
        break;

    case WM_KEYUP:
        switch (LOWORD(wp))
        {
        case 'W':
            camera_unset(CAM_FWD);
            return 0;

        case 'S':
            camera_unset(CAM_BACK);
            return 0;

        case 'A':
            camera_unset(CAM_LEFT);
            return 0;

        case 'D':
            camera_unset(CAM_RIGHT);
            return 0;

        case 'Q':
            camera_unset(CAM_UP);
            return 0;

        case 'E':
            camera_unset(CAM_DOWN); 
            return 0;

        
        case VK_LEFT:
            camera_unset(CAM_TURN_LEFT);
            return 0;

        case VK_RIGHT:
            camera_unset(CAM_TURN_RIGHT);
            return 0;

        case VK_UP:
            camera_unset(CAM_TURN_UP);
            return 0;

        case VK_DOWN:
            camera_unset(CAM_TURN_DOWN);
            return 0;

        }
        break;
    
    case WM_SIZE:
        g_winw = (unsigned int)LOWORD(lp);
        g_winh = (unsigned int)HIWORD(lp);
        draw_viewport(g_winw, g_winh);
        break;

#if 0    
    case WM_CREATE:
        /* Make a rendering timer */
        g_timer = SetTimer(g_wnd, 0, 0, NULL);
        if (!g_timer)
        {
            MessageBoxA(g_wnd, "Failed to create timer!", "Oops!", MB_ICONHAND);
        }
        break;
#endif
    }

    return DefWindowProc(win,msg,wp,lp);
}

