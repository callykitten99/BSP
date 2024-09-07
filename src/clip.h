/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Defines subroutines related to (planar) clipping of geometry in order to
    facilitate compatibility with BSP partitioning.
*******************************************************************************/

#ifndef CLIP_H
#define CLIP_H

#include "pools.h"

typedef struct {
    unsigned short l, r, pl, pr;
} clip_pivot;

bool clip_face(clip_pivot  *pivot,
               pools       *pool,
            unsigned short  face_i,
            unsigned short  clipper_i,
            unsigned short *advance);

#endif /* CLIP_H */
