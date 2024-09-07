#ifndef CLIP_H
#define CLIP_H

#include "pools.h"

#if 0
typedef struct {
    vert clip[2];
    face faces[3];
    bool two : 1; /* True if two faces were created */
    unsigned vsrc : 9;
} clipping;
#endif

typedef struct {
    unsigned short l, r, pl, pr;
} clip_pivot;

#if 0
bool clip_get_verts(clipping const  *const clip,
                       pools const  *const pool,
                       vert        **      out,
               unsigned char               fi);
#endif

bool clip_face(clip_pivot  *pivot,
               pools       *pool,
            unsigned short  face_i,
            unsigned short  clipper_i,
            unsigned short *advance);

#endif /* CLIP_H */
