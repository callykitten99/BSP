#include "camera.h"
#include "math.h"
#include "window.h"

#include <stdbool.h>
#include <string.h>



camera g_cam = {0};
CAM_DIR g_camdir = 0;

double g_proj_e, g_proj_en, g_proj_S, g_proj_Sa;

float g_mat[16] = {0};
float g_proj[16] = {0};
float g_orimat[9] = {0};

bool g_mat_dirty = true;
bool g_proj_dirty = true;
bool g_proj_params_dirty = true;



void
camera_reset(void)
{
    memset(&g_cam, 0, sizeof(g_cam));
    g_cam.fov = 80.0f;
    g_cam.asp = window_get_aspect_ratio();
    g_cam.nearp = 0.1f;
    g_cam.farp  = 128.1f;
}



void
camera_snap(float const *pos)
{
    memcpy(g_cam.pos, pos, sizeof(g_cam.pos));
}

void
camera_turn(float const *ori)
{
    g_cam.ori[0] += ori[0];
    g_cam.ori[1] += ori[1];
    g_cam.ori[2] += ori[2];
}



void
camera_proj_params_clean(void)
{
    g_proj_S = 1.0 / tan(g_cam.fov *
                         0.00872664625997164788461845384244);
    g_proj_Sa = g_proj_S * g_cam.asp;
    g_proj_e = (double)g_cam.farp / ((double)g_cam.farp - (double)g_cam.nearp);
    g_proj_en = -g_proj_e * g_cam.nearp;

    g_proj_params_dirty = false;
    g_proj_dirty        = true;
}



void
camera_proj(float nearp,
            float farp,
            float fov,
            float asp)
{
    g_cam.nearp = nearp;
    g_cam.farp  = farp;
    g_cam.fov   = fov;
    g_cam.asp   = asp;
    
    g_proj_params_dirty = true;
    g_proj_dirty        = true;
}

void
camera_set_near(float nearp)
{
    g_cam.nearp = nearp;
    g_proj_params_dirty = true;
    g_proj_dirty        = true;
}

void
camera_set_far(float farp)
{
    g_cam.farp = farp;
    g_proj_params_dirty = true;
    g_proj_dirty        = true;
}

void
camera_set_fov(float fov)
{
    g_cam.fov = fov;
    g_proj_params_dirty = true;
    g_proj_dirty        = true;
}

void
camera_set_asp(float asp)
{
    g_cam.asp = asp;
    g_proj_params_dirty = true;
    g_proj_dirty        = true;
}



void
camera_matrix_clean(void)
{
#define CXO_P CAM_ORI_PITCH
#define CXO_Y CAM_ORI_YAW
#define CXO_R CAM_ORI_ROLL
    double const xcos[3] = {
            cos(g_cam.ori[CXO_P]),
            cos(g_cam.ori[CXO_Y]),
            cos(g_cam.ori[CXO_R])
        },
        xsin[3] = {
            sin(g_cam.ori[CXO_P]),
            sin(g_cam.ori[CXO_Y]),
            sin(g_cam.ori[CXO_R])
        };

    /* First calculate orientation matrix */
    g_orimat[ 0] = (float)(xcos[CXO_Y]*xcos[CXO_R]);
    g_orimat[ 1] = (float)(xcos[CXO_Y]*xsin[CXO_R]);
    g_orimat[ 2] = (float)(xsin[CXO_Y]);

    g_orimat[ 3] = (float)(-(xsin[CXO_Y]*xsin[CXO_P]*xcos[CXO_R] +
                             xcos[CXO_P]*xsin[CXO_R]));
    g_orimat[ 4] = (float)(-xsin[CXO_Y]*xsin[CXO_P]*xsin[CXO_R] +
                            xcos[CXO_P]*xcos[CXO_R]);
    g_orimat[ 5] = (float)(xcos[CXO_Y]*xsin[CXO_P]);
    g_orimat[ 6] = (float)(-xsin[CXO_Y]*xcos[CXO_P]*xcos[CXO_R] +
                            xsin[CXO_P]*xsin[CXO_R]);
    g_orimat[ 7] = (float)(-(xsin[CXO_Y]*xcos[CXO_P]*xsin[CXO_R] +
                             xsin[CXO_P]*xcos[CXO_R]));
    g_orimat[ 8] = (float)(xcos[CXO_Y]*xcos[CXO_P]);


    /* Then multiply against projection matrix */
#if 0
    g_mat[0] = g_proj_S * g_orimat[0];
    g_mat[4] = g_proj_S * g_orimat[3];
    g_mat[8] = g_proj_S * g_orimat[6];

    g_mat[1] = g_proj_Sa * g_orimat[1];
    g_mat[5] = g_proj_Sa * g_orimat[4];
    g_mat[9] = g_proj_Sa * g_orimat[7];

    g_mat[2] = g_proj_e * g_orimat[2] - g_cam.pos[0];
    g_mat[6] = g_proj_e * g_orimat[5] - g_cam.pos[1];
    g_mat[10]= g_proj_e * g_orimat[8] - g_cam.pos[2];

    g_mat[3] = g_proj_en * g_orimat[2];
    g_mat[7] = g_proj_en * g_orimat[5];
    g_mat[11]= g_proj_en * g_orimat[8];

    g_mat[12]= 0.0f;
    g_mat[13]= 0.0f;
    g_mat[14]=-1.0f;
    g_mat[15]= 0.0f;
#endif

    g_mat[ 0] = g_orimat[ 0];
    g_mat[ 4] = g_orimat[ 1];
    g_mat[ 8] = g_orimat[ 2];
    g_mat[12] =-vec3_dot(g_orimat+0, g_cam.pos);
    g_mat[ 1] = g_orimat[ 3];
    g_mat[ 5] = g_orimat[ 4];
    g_mat[ 9] = g_orimat[ 5];
    g_mat[13] =-vec3_dot(g_orimat+3, g_cam.pos);
    g_mat[ 2] = g_orimat[ 6];
    g_mat[ 6] = g_orimat[ 7];
    g_mat[10] = g_orimat[ 8];
    g_mat[14] =-vec3_dot(g_orimat+6, g_cam.pos);
    g_mat[ 3] = 0.0f;
    g_mat[ 7] = 0.0f;
    g_mat[11] = 0.0f;
    g_mat[15] = 1.0f;
    
    g_mat_dirty = false;
}



void
camera_proj_clean(void)
{
    if (g_proj_params_dirty)
    {
        camera_proj_params_clean();
    }
    
    g_proj[ 0] = (float)g_proj_S; // --
    g_proj[ 1] = 0.0f;
    g_proj[ 2] = 0.0f;
    g_proj[ 3] = 0.0f;
    g_proj[ 4] = 0.0f; // --
    g_proj[ 5] = (float)g_proj_Sa;
    g_proj[ 6] = 0.0f;
    g_proj[ 7] = 0.0f;
    g_proj[ 8] = 0.0f; // --
    g_proj[ 9] = 0.0f;
    g_proj[10] = (float)g_proj_e;
    g_proj[11] = 1.0f;
    g_proj[12] = 0.0f; // --
    g_proj[13] = 0.0f;
    g_proj[14] = (float)g_proj_en;
    g_proj[15] = 0.0f;
    
    g_proj_dirty = false;
}



void
camera_clean(void)
{
    if (g_proj_dirty)
    {
        camera_proj_clean();
    }
    if (g_mat_dirty)
    {
        camera_matrix_clean();
    }
}



void
camera_get_matrix(float *out)
{
    memcpy(out, g_mat, sizeof(g_mat));
}



void
camera_update(float delta)
{
    float const mspeed = delta*10.0f;
    float const rspeed = delta*5.0f;
    float mov[3];
    char x = 0, y = 0, z = 0;

    /* Update matrices */
    camera_clean();

    if (g_camdir & CAM_LEFT ) --x;
    if (g_camdir & CAM_RIGHT) ++x;
    if (g_camdir & CAM_FWD  ) ++z;
    if (g_camdir & CAM_BACK ) --z;
    if (g_camdir & CAM_UP   ) ++y;
    if (g_camdir & CAM_DOWN ) --y;

    mov[0] = mspeed * (x * g_orimat[0] + y * g_orimat[3] + z * g_orimat[6]);
    mov[1] = mspeed * (x * g_orimat[1] + y * g_orimat[4] + z * g_orimat[7]);
    mov[2] = mspeed * (x * g_orimat[2] + y * g_orimat[5] + z * g_orimat[8]);

    g_cam.pos[0] += mov[0];
    g_cam.pos[1] += mov[1];
    g_cam.pos[2] += mov[2];

    x = y = z = 0;
    if (g_camdir & CAM_TURN_LEFT ) ++y;
    if (g_camdir & CAM_TURN_RIGHT) --y;
    if (g_camdir & CAM_TURN_UP   ) --x;
    if (g_camdir & CAM_TURN_DOWN ) ++x;

    mov[0] = x * rspeed;
    mov[1] = y * rspeed;
    mov[2] = z * rspeed;

    g_cam.ori[0] += mov[0];
    g_cam.ori[1] += mov[1];
    g_cam.ori[2] += mov[2];
    
    g_mat_dirty = true;
}



void
camera_set(CAM_DIR d)
{
    g_camdir |= d;
}

void
camera_unset(CAM_DIR d)
{
    g_camdir &= ~d;
}



float
camera_fog_exp_d(void)
{
    return (float)(5.5451774444795624753378569716654 / g_cam.farp);
}

float
camera_fog_exp2_d(void)
{
    return (float)(2.3548200450309493820231386529194 / g_cam.farp);
}

