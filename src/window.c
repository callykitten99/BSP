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

#include <stdio.h>

#ifdef OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#elif defined(OS_LINUX)
#include <unistd.h>
#include <sched.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <GL/gl.h>
#include <GL/glx.h>
#else
#error "OS required"
#endif /* OS selection */

#define CLASS_NAME "ENOBY_BSP"
#define APP_NAME   "Enoby BSP Viewer"



#ifdef OS_WINDOWS
LRESULT CALLBACK WinMsgProc(HWND,UINT,WPARAM,LPARAM);
#elif defined(OS_LINUX)
typedef struct {
    long left, top, right, bottom;
} RECT;

// Don't make XEvent const
bool XMsgProc(XEvent *msg);
#endif



extern pools g_pool;
extern bsp   g_bsp;
extern unsigned short
    g_bsp_l, g_bsp_r, g_pivot_l, g_pivot_r, g_pivot,
    g_poly_clip, g_poly_inter, g_vert_012[3];

extern DRAW_MODE g_draw_mode;
extern float g_ray[3];

#ifdef OS_WINDOWS
LARGE_INTEGER f;
LARGE_INTEGER t0, t1;
HWND          g_wnd;
#elif defined(OS_LINUX)
struct timespec t0, t1;

Display *g_display;
Atom     g_wm_delete_window;
int      g_screen;
Window   g_wnd;
Colormap g_colorMap;
GLXContext g_glrc;
#endif

unsigned int g_winw, g_winh;

bool g_lapse = false,
     g_stall = false,
     g_scheduleSelect = false;



#ifdef OS_WINDOWS
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

    unsigned long black, white;

    
}
#endif



void
centred_window_rect(unsigned int w,
                    unsigned int h,
                    RECT *out)
{
#ifdef OS_WINDOWS
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
#elif defined(OS_LINUX)
    Screen *scr = XScreenOfDisplay(g_display, g_screen);
    int scrw, scrh;

    scrw = XWidthOfScreen (scr);
    scrh = XHeightOfScreen(scr);
    out->left   = (scrw>>1) - (w>>1);
    out->right  = out->left + w;
    out->top    = (scrh>>1) - (h>>1);
    out->bottom = out->top + h;
#endif
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
#ifdef OS_LINUX
    XVisualInfo  *vi;
    Window        root;
    XSetWindowAttributes swa;
    GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
#endif

    /* Update globals */
#ifdef OS_WINDOWS
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    t1 = t0;
#elif defined(OS_LINUX)
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t0);
    t1 = t0;

    g_display = XOpenDisplay(NULL);
    if (!g_display)
    {
        fprintf(stderr, "Failed to open X display.\n");
        return false;
    }
    g_screen = DefaultScreen(g_display);
#endif
    g_winw = w;
    g_winh = h;

#ifdef OS_WINDOWS
    if (!window_class_init())
    {
        fprintf(stderr, "Failed to create window class.\n");
        return false;
    }
#elif defined(OS_LINUX)
    root = DefaultRootWindow(g_display);

    vi = glXChooseVisual(g_display, 0, att);
    if (!vi)
    {
        fprintf(stderr, "No appropriate OpenGL visual descriptor found for "
                        "creating window.\n");
        return false;
    }

    g_colorMap = XCreateColormap(g_display, root, vi->visual, AllocNone);
    swa.colormap = g_colorMap;
    swa.event_mask = ExposureMask    | StructureNotifyMask |
                     KeyPressMask    | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask;
#endif

    centred_window_rect(w,h,&rct);

#ifdef OS_WINDOWS
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
#elif defined(OS_LINUX)
    g_wnd = XCreateWindow(g_display, root,
        rct.left, rct.top,
        rct.right - rct.left,
        rct.bottom - rct.top,
        0, vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa);
#endif

    if (!g_wnd)
    {
        fprintf(stderr, "Failed to create window.\n");
        return false;
    }

#ifdef OS_LINUX
    XStoreName(g_display, g_wnd, APP_NAME);
    XMapRaised(g_display, g_wnd);

    g_wm_delete_window = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    if (g_wm_delete_window == None)
    {
        fprintf(stderr, "Failed to register window deletion event atom.\n");
        return false;
    }
    XSetWMProtocols(g_display, g_wnd, &g_wm_delete_window, 1);

    g_glrc = glXCreateContext(g_display, vi, NULL, GL_TRUE);
    if (!g_glrc)
    {
        fprintf(stderr, "Failed to create OpenGL X11 rendering context.\n");
        return false;
    }
    glXMakeCurrent(g_display, g_wnd, g_glrc);
#endif

    return true;
}



void
window_cleanup(void)
{
#ifdef OS_WINDOWS
    if (g_wnd)
    {
        DestroyWindow(g_wnd);
        g_wnd = NULL;
    }
#elif defined OS_LINUX
    if (g_display)
    {
        if (g_glrc)
        {
            glXDestroyContext(g_display, g_glrc);
            g_glrc = NULL;
        }
        if (g_wnd)
        {
            XDestroyWindow(g_display, g_wnd);
            g_wnd = 0;
        }
        if (g_colorMap)
        {
            XFreeColormap(g_display, g_colorMap);
            g_colorMap = 0;
        }

        XCloseDisplay (g_display);
        g_display = NULL;
    }
    #endif
}



float
window_get_aspect_ratio(void)
{
    return (float)((double)g_winw / (double)g_winh);
}



bool
window_loop(void)
{
#ifdef OS_WINDOWS
    MSG msg = {0};
#elif defined(OS_LINUX)
    XEvent msg = {0};
    unsigned int numEvents;
#endif
    float delta;

#ifdef OS_WINDOWS
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

#elif defined(OS_LINUX)
    /* Get messages */
    numEvents = XPending(g_display);
    while (numEvents--)
    {
        XNextEvent(g_display, &msg);
        if (!XMsgProc(&msg)) return false;
    }

    /* Get time elapsed */
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
    delta = (float)
    (
        (long double) (
            ((long long)t1.tv_sec * (long long)1000000000ULL +
                                    (long long)t1.tv_nsec)
           -((long long)t0.tv_sec * (long long)1000000000ULL +
                                    (long long)t0.tv_nsec)
        ) / (long double)1000000000.0L
    );
#endif
    
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
        {
#ifdef OS_WINDOWS
            Sleep(5);
#elif defined(OS_LINUX)
            sched_yield();
            //nanosleep(1);
#endif
        }
        else
        {
#ifdef OS_WINDOWS
            Sleep(0);
#elif defined(OS_LINUX)
            sched_yield();
            //nanosleep(1);
#endif
        }
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



#ifdef OS_WINDOWS
#define KEYCODE_ESCAPE VK_ESCAPE
#define KEYCODE_RETURN VK_RETURN
#define KEYCODE_SPACE  VK_SPACE
#define KEYCODE_LEFT   VK_LEFT
#define KEYCODE_RIGHT  VK_RIGHT
#define KEYCODE_UP     VK_UP
#define KEYCODE_DOWN   VK_DOWN
#define KEYCODE_PLUS   VK_OEM_PLUS
#define KEYCODE_MINUS  VK_OEM_MINUS
#define KEYCODE_1      '1'
#define KEYCODE_2      '2'
#define KEYCODE_3      '3'
#define KEYCODE_4      '4'
#define KEYCODE_q      'Q'
#define KEYCODE_w      'W'
#define KEYCODE_e      'E'
#define KEYCODE_r      'R'
#define KEYCODE_t      'T'
#define KEYCODE_i      'I'
#define KEYCODE_p      'P'
#define KEYCODE_LSB    '['
#define KEYCODE_RSB    ']'
#define KEYCODE_a      'A'
#define KEYCODE_s      'S'
#define KEYCODE_d      'D'
#define KEYCODE_f      'F'
#define KEYCODE_g      'G'
#define KEYCODE_h      'H'
#define KEYCODE_j      'J'
#define KEYCODE_k      'K'
#define KEYCODE_v      'V'
#define KEYCODE_b      'B'
#elif defined(OS_LINUX)
#define KEYCODE_ESCAPE XK_Escape
#define KEYCODE_RETURN XK_Return
#define KEYCODE_SPACE  XK_space
#define KEYCODE_LEFT   XK_Left
#define KEYCODE_RIGHT  XK_Right
#define KEYCODE_UP     XK_Up
#define KEYCODE_DOWN   XK_Down
#define KEYCODE_PLUS   XK_plus 
#define KEYCODE_MINUS  XK_minus
#define KEYCODE_1      XK_1
#define KEYCODE_2      XK_2
#define KEYCODE_3      XK_3
#define KEYCODE_4      XK_4
#define KEYCODE_q      XK_q
#define KEYCODE_w      XK_w
#define KEYCODE_e      XK_e
#define KEYCODE_r      XK_r
#define KEYCODE_t      XK_t
#define KEYCODE_i      XK_i
#define KEYCODE_p      XK_p
#define KEYCODE_LSB    XK_bracketleft
#define KEYCODE_RSB    XK_bracketright
#define KEYCODE_a      XK_a
#define KEYCODE_s      XK_s
#define KEYCODE_d      XK_d
#define KEYCODE_f      XK_f
#define KEYCODE_g      XK_g
#define KEYCODE_h      XK_h
#define KEYCODE_j      XK_j
#define KEYCODE_k      XK_k
#define KEYCODE_v      XK_v
#define KEYCODE_b      XK_b
#endif



static bool
MsgKeyDown(int keycode)
{
    switch (keycode)
    {
    case KEYCODE_LSB:
        pivot_next();
        break;

    case KEYCODE_RSB:
        pivot_prev();
        break;

    case KEYCODE_1:
        g_draw_mode = DRAW_MODE_LR;
        break;
    
    case KEYCODE_2:
        g_draw_mode = DRAW_MODE_REL;
        break;
    
    case KEYCODE_3:
        g_draw_mode = DRAW_MODE_CLIP;
        break;
    
    case KEYCODE_4:
        g_draw_mode = DRAW_MODE_BSP;
        break;
        
    case KEYCODE_a:
        camera_set(CAM_LEFT);
        break;
    
    case KEYCODE_d:
        camera_set(CAM_RIGHT);
        break;

    case KEYCODE_w:
        camera_set(CAM_FWD);
        break;

    case KEYCODE_s:
        camera_set(CAM_BACK);
        break;

    case KEYCODE_q:
        camera_set(CAM_UP);
        break;

    case KEYCODE_e:
        camera_set(CAM_DOWN);
        break;
    
    case KEYCODE_t:
        g_ray[2] += 0.1f;
        break;
    case KEYCODE_g:
        g_ray[2] -= 0.1f;
        break;
    case KEYCODE_f:
        g_ray[0] -= 0.1f;
        break;
    case KEYCODE_h:
        g_ray[0] += 0.1f;
        break;
    case KEYCODE_v:
        g_ray[1] -= 0.1f;
        break;
    case KEYCODE_b:
        g_ray[1] += 0.1f;
        break;

    case KEYCODE_p:
        /* BSP display: go to root node */
        if (g_bsp.d)
        {
            g_pivot_l = g_bsp.d[0].pl;
            g_pivot_r = g_bsp.d[0].pr;
            g_pivot = 0;
            g_bsp_l = 0;
            g_bsp_r = -1;
        }
        break;

    case KEYCODE_i:
        /* BSP display: go to parent node */
        if (g_bsp.d && g_pivot < g_bsp.occ)
        {
            bsp_ind next = g_bsp.d[g_pivot].p;
            if (next < g_bsp.occ)
            {
                g_pivot = next;
                g_pivot_l = g_bsp.d[next].pl;
                g_pivot_r = g_bsp.d[next].pr;
                return true;
            }
            else
            {
                g_pivot_l = g_bsp.d[0].pl;
                g_pivot_r = g_bsp.d[0].pr;
                g_pivot = 0;
                g_bsp_l = 0;
                g_bsp_r = -1;
                return true;
            }
        }
        g_pivot = -1;
        break;

    case KEYCODE_j:
        /* BSP display: go to left child */
        if (g_bsp.d && g_pivot < g_bsp.occ)
        {
            bsp_ind next = g_bsp.d[g_pivot].l;
            if (next < g_bsp.occ)
            {
                g_pivot = next;
                g_pivot_l = g_bsp.d[next].pl;
                g_pivot_r = g_bsp.d[next].pr;
                return true;
            }
            return true;
        }
        g_pivot = -1;
        break;

    case KEYCODE_k:
        /* BSP Display: go to right child */
        if (g_bsp.d && g_pivot < g_bsp.occ)
        {
            bsp_ind next = g_bsp.d[g_pivot].r;
            if (next < g_bsp.occ)
            {
                g_pivot = next;
                g_pivot_l = g_bsp.d[next].pl;
                g_pivot_r = g_bsp.d[next].pr;
                return true;
            }
            return true;
        }
        g_pivot = -1;
        break;

    case KEYCODE_MINUS:
        camera_set_far(g_cam.farp - 1.f);
        break;

    case KEYCODE_PLUS:
        camera_set_far(g_cam.farp + 1.f);
        break;

    case KEYCODE_LEFT:
        camera_set(CAM_TURN_LEFT);
        break;

    case KEYCODE_RIGHT:
        camera_set(CAM_TURN_RIGHT);
        break;

    case KEYCODE_UP:
        camera_set(CAM_TURN_UP);
        break;

    case KEYCODE_DOWN:
        camera_set(CAM_TURN_DOWN);
        break;

    case KEYCODE_RETURN:
        g_stall = false;
        break;

    case KEYCODE_SPACE:
        if (!g_stall)
            g_scheduleSelect = true;            
        break;

    case KEYCODE_ESCAPE:
#ifdef OS_WINDOWS
        PostQuitMessage(0);
#elif defined(OS_LINUX)
        if (1)
        {
            XEvent msg = {0};

            msg.type = ClientMessage;
            //msg.display = g_display;
            //msg.window  = g_wnd;
            //msg.xclient.message_type = g_wm_delete_window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = g_wm_delete_window;

            XSendEvent(g_display, g_wnd, false, NoEventMask, &msg);
        }
#endif
        break;

    case KEYCODE_r:
        camera_reset();
        break;

    /* This message was not processed */
    default: return false;
    }

    /* This message was processed */
    return true;
}



static bool
MsgKeyUp(int keycode)
{
    switch (keycode)
    {
    case KEYCODE_w:
        camera_unset(CAM_FWD);
        break;

    case KEYCODE_s:
        camera_unset(CAM_BACK);
        break;

    case KEYCODE_a:
        camera_unset(CAM_LEFT);
        break;

    case KEYCODE_d:
        camera_unset(CAM_RIGHT);
        break;

    case KEYCODE_q:
        camera_unset(CAM_UP);
        break;

    case KEYCODE_e:
        camera_unset(CAM_DOWN); 
        break;


    case KEYCODE_LEFT:
        camera_unset(CAM_TURN_LEFT);
        break;

    case KEYCODE_RIGHT:
        camera_unset(CAM_TURN_RIGHT);
        break;

    case KEYCODE_UP:
        camera_unset(CAM_TURN_UP);
        break;

    case KEYCODE_DOWN:
        camera_unset(CAM_TURN_DOWN);
        break;

    default: return false;
    }

    return true;
}



#ifdef OS_WINDOWS
LRESULT CALLBACK
WinMsgProc(HWND win,
           UINT msg,
           WPARAM wp,
           LPARAM lp)
{
    switch (msg)
    {
    case WM_CLOSE:
        /* Exit program */
        PostQuitMessage(0);
        return 0;

    case WM_CHAR:
        if (!(lp & (1<<31)) && MsgKeyDown(LOWORD(wp)))
        {
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (MsgKeyDown(LOWORD(wp)))
        {
            return 0;
        }
        break;

    case WM_KEYUP:
        if (MsgKeyUp(LOWORD(wp)))
        {
            return 0;
        }
        break;
    
    case WM_SIZE:
        g_winw = (unsigned int)LOWORD(lp);
        g_winh = (unsigned int)HIWORD(lp);
        draw_viewport(g_winw, g_winh);
        break;
    }

    return DefWindowProc(win,msg,wp,lp);
}
#endif



#ifdef OS_LINUX
bool
XMsgProc(XEvent *msg)
{
    char buf[8];
    KeySym ks;

    switch (msg->type)
    {
    case KeyPress:
        //ks = XLookupKeysym(&msg->xkey, 0);
        XLookupString(&msg->xkey, buf, 4, &ks, NULL);
        if (ks != NoSymbol) MsgKeyDown(ks);
        break;

    case KeyRelease:
        ks = XLookupKeysym(&msg->xkey, 0);
        if (ks != NoSymbol) MsgKeyUp(ks);
        break;

    case ClientMessage:
        if ((Atom)msg->xclient.data.l[0] == g_wm_delete_window)
        {
            /* Exit program */
            return false;
        }
        break;

    case ConfigureNotify:
        g_winw = (unsigned int)msg->xconfigure.width;
        g_winh = (unsigned int)msg->xconfigure.height;
        draw_viewport(g_winw, g_winh);
        break;
    }

    /* Don't exit program */
    return true;
}
#endif
