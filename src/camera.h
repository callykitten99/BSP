#ifndef CAMERA_H
#define CAMERA_H

#define CAM_ORI_PITCH 0
#define CAM_ORI_YAW   1
#define CAM_ORI_ROLL  2

typedef enum {
    CAM_FWD   = 1,
    CAM_BACK  = 2,
    CAM_LEFT  = 4,
    CAM_RIGHT = 8,
    CAM_UP    = 16,
    CAM_DOWN  = 32,

    CAM_TURN_LEFT = 64,
    CAM_TURN_RIGHT = 128,
    CAM_TURN_UP = 256,
    CAM_TURN_DOWN = 512,
} CAM_DIR;

typedef struct camera {
    float pos[3];
    float ori[3];
    float nearp, farp, fov, asp;
} camera;

extern camera g_cam;
extern float g_proj[16];
extern float g_mat[16];

void camera_reset(void); /* To origin */
void camera_update(float delta);

void camera_set_proj(float nearp, float farp, float fov, float asp);
void camera_set_near(float nearp);
void camera_set_far (float farp );
void camera_set_fov (float fov  );
void camera_set_asp (float asp  );

void camera_set  (CAM_DIR);
void camera_unset(CAM_DIR);

void camera_snap (float const *pos);
void camera_turn (float const *ori);

void camera_clean(void);
void camera_get_matrix(float *out);

static inline float  *camera_ptr_matrix     (void) { return g_mat; }
static inline float  *camera_ptr_proj_matrix(void) { return g_proj; }
static inline camera *camera_get            (void) { return &g_cam; }



float camera_fog_exp_d (void);
float camera_fog_exp2_d(void);

#endif /* CAMERA_H */
