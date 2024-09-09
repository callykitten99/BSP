/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Controls rendering of geometry (via OpenGL).
*******************************************************************************/

#include "camera.h"
#include "draw.h"
#include "select.h"
#include "math.h"
#include "bsp.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>

#ifdef OS_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#elif defined(OS_LINUX)
#include <unistd.h>
#include <GL/glx.h>
#include <GL/glu.h>
#endif



#ifdef OS_WINDOWS
extern LRESULT CALLBACK WinMsgProc(HWND,UINT,WPARAM,LPARAM);
extern HWND g_wnd;
#elif defined OS_LINUX
extern Display   *g_display;
extern Window     g_wnd;
extern GLXContext g_glrc;
#endif



extern pools g_pool;
extern bsp   g_bsp;

extern unsigned int g_winw, g_winh;

extern bool g_lapse, g_stall;

float g_ray[3] = {0.0f,0.0f,0.0f};

#ifdef OS_WINDOWS
HGLRC    g_glrc = NULL;
HDC      g_dc   = NULL;
#endif

unsigned short
    g_bsp_l = 0, g_bsp_r = -1, g_pivot_l = 0, g_pivot_r = 0, g_pivot = -1,
    g_poly_clip = 0, g_poly_inter = 0, g_vert_012[3] = {0};

DRAW_MODE g_draw_mode = DRAW_MODE_UNSPECIFIED;



void
draw_cleanup(void)
{
#ifdef OS_WINDOWS
    if (g_glrc)
    {
        wglDeleteContext(g_glrc);
        g_glrc = NULL;
    }
#endif
}



bool
draw_init(void)
{
#ifdef OS_WINDOWS

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(pfd),
        1, /* Version */
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, /* RGBA or palette */
        32, /* Colour depth */
        0, 0, /* Red bits + shift */
        0, 0, /* Green bits + shift */
        0, 0, /* Blue bits + shift */
        0, 0, /* Alpha bits + shift */
        0, 0, 0, 0, 0, /* Accum bits, red ", green ", blue ", alpha " */
        24, /* Depth buffer bits */
        8,  /* Stencil buffer bits */
        0, /* Number of auxiliary buffers */
        PFD_MAIN_PLANE, /* Layer type */
        0, /* Reserved */
        0, 0, 0 /* Layer mask, visible mask, damage mask */
    };

    HDC dc = GetDC(g_wnd);

    int pf = ChoosePixelFormat(dc, &pfd);


    if (!pf)
    {
        ReleaseDC(g_wnd, dc);
        fprintf(stderr, "Failed to match OpenGL pixel format.\n");
        return false;
    }

    SetPixelFormat(dc, pf, &pfd);

    g_glrc = wglCreateContext(dc);
    wglMakeCurrent(dc, g_glrc);
    ReleaseDC(g_wnd, dc);

#endif /* OS_WINDOWS */

    //fprintf(stderr, "Using OpenGL version %s.\n", glGetString(GL_VERSION));

    /* GL rendering params */
    glViewport(0,0, g_winw, g_winh);
    glClearDepth(1.0f);
    camera_reset();
    camera_snap(g_pool.verts[0].m);

    return true;
}



static void draw_LR  (void);
static void draw_rel (void);
static void draw_clip(void);
static void draw_bsp (void);

void
draw(DRAW_MODE dm)
{
    bool restart = true;
    for (;;) switch (dm)
    {
    case DRAW_MODE_LR:   draw_LR();   return;
    case DRAW_MODE_REL:  draw_rel();  return;
    case DRAW_MODE_CLIP: draw_clip(); return;
    case DRAW_MODE_BSP:  draw_bsp();  return;
    default:
        if (restart)
        {
            restart = false;
            dm = g_draw_mode;
            continue;
        }
        g_draw_mode = DRAW_MODE_LR;
        draw_LR();
        return;
    }
}



static void
draw_LR(void)
{
    static const float clearCol[4] = { 0.2f, 0.1f, 0.4f, 1.0f };

#ifdef OS_WINDOWS
    HDC dc = g_dc ? g_dc : GetDC(g_wnd);
#endif

    vert *verts = g_pool.verts;
    face *faces = g_pool.faces;
    unsigned short i, l, pl, pr, r;

    glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, camera_fog_exp2_d());
    glFogfv(GL_FOG_COLOR, clearCol);
    glFrontFace(GL_CW);
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_LINE);
    //glCullFace(GL_BACK);
    

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_ptr_proj_matrix());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(camera_ptr_matrix());
    
    
    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f,1.0f,1.0f);   glVertex3fv(g_ray);
    glEnd();
    
    
    glBegin(GL_TRIANGLES);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    l = g_bsp_l < g_pool.n_faces ? g_bsp_l : g_pool.n_faces;
    pl = g_pivot_l < g_pool.n_faces ? g_pivot_l : g_pool.n_faces;
    pr = g_pivot_r < g_pool.n_faces ? g_pivot_r : g_pool.n_faces;
    r = g_bsp_r < g_pool.n_faces ? g_bsp_r : g_pool.n_faces;

    /* Draw polys outside of BSP tree range */
    /*for (i = 0; i < l; ++i)
    {
        glColor3f(0.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(.25f, .25f, .25f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(0.5f, 0.5f, 0.5f);    glVertex3fv(verts[faces[i].i[2]].m);
    }*/
    
    /* Draw polys left of the BSP pivot */
    for (i = l; i < pl; ++i)
    {
        glColor3f(0.5f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(1.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(.75f, .25f, .25f);    glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw the pivots */
    for (i = pl; i <= pr; ++i)
    {
        glColor3f(0.0f, 0.5f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(0.0f, 1.0f, 0.0f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(.25f, .75f, .25f);    glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw polys on right of the BSP pivot */
    for (i = pr+1; i < r; ++i)
    {
        glColor3f(0.0f, 0.0f, 0.5f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(0.0f, 0.0f, 1.0f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(.25f, .25f, .75f);    glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw polys outside of the BSP tree range */
    /*for (i = r; i < g_pool.n_faces; ++i)
    {
        glColor3f(0.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(.25f, .25f, .25f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(0.5f, 0.5f, 0.5f);    glVertex3fv(verts[faces[i].i[2]].m);
    }*/

    glEnd();

#ifdef OS_WINDOWS
    SwapBuffers(dc);
    if (!g_dc)
        ReleaseDC(g_wnd, dc);
#elif defined OS_LINUX
    glXSwapBuffers(g_display, g_wnd);
#endif
}



static float const *
draw_rel_colour(PLANE_REL rel)
{
    static const float colours[4][9] =
    {
        { 0.5f, 0.0f, 0.0f,
          1.0f, 0.0f, 0.0f,
          .75f, .25f, .25f }, // Red (LEFT)
        
        { 0.0f, 0.0f, 0.5f,
          0.0f, 0.0f, 1.0f,
          .25f, .25f, .75f }, // Blue (RIGHT)
        
        { 1.0f, 1.0f, 0.0f,
          .75f, .75f, .25f,
          0.5f, 0.5f, 0.0f }, // Yellow (INTERSECTING)

        { 0.0f, 0.5f, 0.0f,
          0.0f, 1.0f, 0.0f,
          .25f, .75f, .25f }, // Green (COINCIDING)
    };

    return &colours[rel][0];
}



static void
draw_rel(void)
{
    static const float clearCol[4] = { 0.2f, 0.1f, 0.4f, 1.0f };

#ifdef OS_WINDOWS
    HDC dc = g_dc ? g_dc : GetDC(g_wnd);
#endif

    vert  *verts  = g_pool.verts;
    face  *faces  = g_pool.faces;
    plane *planes = g_pool.planes;
    unsigned short i, l, pl, pr, r;

    glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, camera_fog_exp2_d());
    glFogfv(GL_FOG_COLOR, clearCol);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_ptr_proj_matrix());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(camera_ptr_matrix());
    
    
    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f,1.0f,1.0f);   glVertex3fv(g_ray);
    glEnd();
    
    
    glBegin(GL_TRIANGLES);
    
    l  = g_bsp_l   < g_pool.n_faces ? g_bsp_l   : (g_pool.n_faces - 1);
    pl = g_pivot_l < g_pool.n_faces ? g_pivot_l : (g_pool.n_faces - 1);
    pr = g_pivot_r < g_pool.n_faces ? g_pivot_r : (g_pool.n_faces - 1);
    r  = g_bsp_r   < g_pool.n_faces ? g_bsp_r   : (g_pool.n_faces - 1);

    /* Draw polys outside of BSP tree range */
    /*for (i = 0; i < l; ++i)
    {
        glColor3f(0.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(.25f, .25f, .25f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(0.5f, 0.5f, 0.5f);    glVertex3fv(verts[faces[i].i[2]].m);
    }*/
    
    /* Draw polys left of the BSP pivot */
    for (i = l; i < pl; ++i)
    {
        float const *colour = draw_rel_colour(planes[i].rel);
        glColor3fv(colour);     glVertex3fv(verts[faces[i].i[0]].m);
        glColor3fv(colour+3);   glVertex3fv(verts[faces[i].i[1]].m);
        glColor3fv(colour+6);   glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw the pivots */
    for (i = pl; i <= pr; ++i)
    {
        float const *colour = draw_rel_colour(planes[i].rel);
        glColor3fv(colour);     glVertex3fv(verts[faces[i].i[0]].m);
        glColor3fv(colour+3);   glVertex3fv(verts[faces[i].i[1]].m);
        glColor3fv(colour+6);   glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw polys on right of the BSP pivot */
    for (i = pr+1; i < r; ++i)
    {
        float const *colour = draw_rel_colour(planes[i].rel);
        glColor3fv(colour);     glVertex3fv(verts[faces[i].i[0]].m);
        glColor3fv(colour+3);   glVertex3fv(verts[faces[i].i[1]].m);
        glColor3fv(colour+6);   glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    /* Draw polys outside of the BSP tree range */
    /*for (i = r; i < g_pool.n_faces; ++i)
    {
        glColor3f(0.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(.25f, .25f, .25f);    glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(0.5f, 0.5f, 0.5f);    glVertex3fv(verts[faces[i].i[2]].m);
    }*/

    glEnd();

#ifdef OS_WINDOWS
    SwapBuffers(dc);
    if (!g_dc)
        ReleaseDC(g_wnd, dc);
#elif defined OS_LINUX
    glXSwapBuffers(g_display, g_wnd);
#endif
}



static void
draw_part(bsp_ind ind, bool t)
{
    static const float col[3][3] =
    {
        { 0.0f, 0.0f, 0.0f },
        { .25f, .25f, .25f },
        { 0.5f, 0.5f, 0.5f },
    };
    
    bsp_node const *pt;
    
    vert  *verts  = g_pool.verts;
    face  *faces  = g_pool.faces;
    plane *planes = g_pool.planes;
    
    float d;
    float projected[3];
    
    unsigned short i;
    
    pt = bsp_node_from_id(&g_bsp, ind);
    if (!pt) return;
    
    if (t)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_TRIANGLES);
        for (i = pt->pl; i <= pt->pr; ++i)
        {
            glColor3fv(&col[0][0]);    glVertex3fv(verts[faces[i].i[0]].m);
            glColor3fv(&col[1][0]);    glVertex3fv(verts[faces[i].i[1]].m);
            glColor3fv(&col[2][0]);    glVertex3fv(verts[faces[i].i[2]].m);
        }
        glEnd();
        
        /* Determine which side of the plane the camera is on */
        d = vec3_project_plane_get_d(planes + pt->pl, projected, g_ray);
        if (d <= -FLT_EPSILON)
        {
            glBegin(GL_LINES);
                glColor3f(1.0f, 0.0f, 0.0f);
                glVertex3fv(g_ray);
                glVertex3fv(projected);
            glEnd();
            
            draw_part(pt->l, true);
            draw_part(pt->r, false);
        }
        else if (d >= FLT_EPSILON)
        {
            glBegin(GL_LINES);
                glColor3f(0.0f, 1.0f, 0.0f);
                glVertex3fv(g_ray);
                glVertex3fv(projected);
            glEnd();
            
            draw_part(pt->l, false);
            draw_part(pt->r, true);
        }
        else
        {
            draw_part(pt->l, true);
            draw_part(pt->r, true);
        }
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin(GL_TRIANGLES);
        for (i = pt->pl; i <= pt->pr; ++i)
        {
            glColor3fv(&col[0][0]);    glVertex3fv(verts[faces[i].i[0]].m);
            glColor3fv(&col[1][0]);    glVertex3fv(verts[faces[i].i[1]].m);
            glColor3fv(&col[2][0]);    glVertex3fv(verts[faces[i].i[2]].m);
        }
        glEnd();
        
        draw_part(pt->l, false);
        draw_part(pt->r, false);
    }
}



static void
draw_bsp(void)
{
#ifdef OS_WINDOWS
    HDC dc = g_dc ? g_dc : GetDC(g_wnd);
#endif

    glClearColor(0.4f, 0.1f, 0.2f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_ptr_proj_matrix());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf(camera_ptr_matrix());
    
    glPointSize(10.0f);
    glBegin(GL_POINTS);
    glColor3f(1.0f,1.0f,1.0f);   glVertex3fv(g_ray);
    glEnd();

    /* Traverse the BSP */
    draw_part(0, true);

#ifdef OS_WINDOWS
    SwapBuffers(dc);
    if (!g_dc)
        ReleaseDC(g_wnd, dc);
#elif defined OS_LINUX
    glXSwapBuffers(g_display, g_wnd);
#endif
}



static void
draw_clip(void)
{
#ifdef OS_WINDOWS
    HDC dc = g_dc ? g_dc : GetDC(g_wnd);
#endif
    vert *verts = g_pool.verts;
    face *faces = g_pool.faces;
    
    unsigned short clipper, poly, v0, v1, v2, i;

    glClearColor(0.2f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    
    glPointSize(5.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_ptr_proj_matrix());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLoadMatrixf(camera_ptr_matrix());
    
    /* Correct indices */
    clipper = g_poly_clip   < g_pool.n_faces ? g_poly_clip   : 0xFFFF;
    poly    = g_poly_inter  < g_pool.n_faces ? g_poly_inter  : 0xFFFF;
    v0      = g_vert_012[0] < g_pool.n_verts ? g_vert_012[0] : 0xFFFF;
    v1      = g_vert_012[1] < g_pool.n_verts ? g_vert_012[1] : 0xFFFF;
    v2      = g_vert_012[2] < g_pool.n_verts ? g_vert_012[2] : 0xFFFF;
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    
    /* Draw all polygons */
    for (i = 0; i < g_pool.n_faces; ++i)
    {
        glColor3f(0.0f, 1.0f, 1.0f);  glVertex3fv(verts[faces[i].i[0]].m);
        glColor3f(0.0f, 0.5f, 0.5f);  glVertex3fv(verts[faces[i].i[1]].m);
        glColor3f(.25f, .75f, .75f);  glVertex3fv(verts[faces[i].i[2]].m);
    }
    
    glEnd();
    
    glClear(GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_BACK, GL_LINE);
    glBegin(GL_TRIANGLES);
    
    /* Draw clipper */
    if (clipper != 0xFFFF)
    {
    glColor3f(0.0f, 0.0f, 0.0f);    glVertex3fv(verts[faces[clipper].i[0]].m);
    glColor3f(.25f, .25f, .25f);    glVertex3fv(verts[faces[clipper].i[1]].m);
    glColor3f(0.5f, 0.5f, 0.5f);    glVertex3fv(verts[faces[clipper].i[2]].m);
    }
    
    /* Draw poly (to clip) */
    if (poly != 0xFFFF)
    {
    glColor3f(1.0f, 1.0f, 0.0f);    glVertex3fv(verts[faces[poly].i[0]].m);
    glColor3f(.75f, .75f, .25f);    glVertex3fv(verts[faces[poly].i[1]].m);
    glColor3f(0.5f, 0.5f, 0.0f);    glVertex3fv(verts[faces[poly].i[2]].m);
    }
    
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    /* Draw vertex order */
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_POINTS);
    
    if (v0 != 0xFFFF) { glColor3f(1.0f, 0.0f, 0.0f); glVertex3fv(verts[v0].m); }
    if (v1 != 0xFFFF) { glColor3f(0.0f, 1.0f, 0.0f); glVertex3fv(verts[v1].m); }
    if (v2 != 0xFFFF) { glColor3f(0.0f, 0.0f, 1.0f); glVertex3fv(verts[v2].m); }
    
    glEnd();
    
#ifdef OS_WINDOWS
    SwapBuffers(dc);
    if (!g_dc)
        ReleaseDC(g_wnd, dc);
#elif defined OS_LINUX
    glXSwapBuffers(g_display, g_wnd);
#endif
}



void
draw_pause(void)
{
    g_stall = true;
    while (g_stall)
    {
        if (!window_loop()) exit(0);
        //Sleep(1);
    }
}



void
draw_delay(unsigned long delay)
{
    draw(DRAW_MODE_UNSPECIFIED);

#ifdef OS_WINDOWS
    Sleep(delay);
#elif defined(OS_LINUX)
    usleep(delay*1000U);
#endif
}



void
draw_viewport(unsigned int w, unsigned int h)
{
    glViewport(0,0, w, h);
    camera_set_asp(window_get_aspect_ratio());
}

