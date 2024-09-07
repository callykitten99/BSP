#ifndef PLANE_H
#define PLANE_H

#ifndef PLANE_REL_DEF
#define PLANE_REL_DEF
typedef enum {
    PLANE_REL_LEFT = 0,
    PLANE_REL_RIGHT,
    PLANE_REL_INTER,     PLANE_REL_INTERSECT = PLANE_REL_INTER,
    PLANE_REL_COINCIDE,
} PLANE_REL;
#endif

typedef struct {
    union {
        float m[3];
        struct {
            float x, y, z;
        };
    };
    float d;
    PLANE_REL rel; /* Used for DRAW_MODE_REL */
} plane;

#endif /* PLANE_H */
