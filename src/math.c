/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Common mathematical and geometric functions and operations.
*******************************************************************************/

#include "math.h"
#include "verbose.h"

#include <stdio.h>



bool
plane_from_face(
    plane *out,
    vert const *verts,
    face *f)
{
    vert const *vs[3];
    double e0[3], e1[3], n[3], mag;

    if (!out || !verts || !f) return false;

    /* Faces wound clockwise are considered 'front facing' */
    vs[0] = verts+f->i[0];
    vs[1] = verts+f->i[2];
    vs[2] = verts+f->i[1];

    /* Calculate edges */
    e0[0] = (double)vs[1]->m[0] - (double)vs[0]->m[0];
    e0[1] = (double)vs[1]->m[1] - (double)vs[0]->m[1];
    e0[2] = (double)vs[1]->m[2] - (double)vs[0]->m[2];
    
    e1[0] = (double)vs[2]->m[0] - (double)vs[0]->m[0];
    e1[1] = (double)vs[2]->m[1] - (double)vs[0]->m[1];
    e1[2] = (double)vs[2]->m[2] - (double)vs[0]->m[2];

    /* Take the cross product */
    n[0] = (e0[1]*e1[2]) - (e0[2]*e1[1]);
    n[1] = (e0[2]*e1[0]) - (e0[0]*e1[2]);
    n[2] = (e0[0]*e1[1]) - (e0[1]*e1[0]);
    
    /* Normalise */
    mag = (n[0]*n[0]) + (n[1]*n[1]) + (n[2]*n[2]);
    if (mag < DBL_EPSILON)
    {
        out->m[0] = 0.0f;
        out->m[1] = 0.0f;
        out->m[2] = 0.0f;
        out->d = 0.0f;
        return false;
    }
    mag = sqrt(mag);
    n[0] /= mag;
    n[1] /= mag;
    n[2] /= mag;

    /* Work out planar distance from origin */
    mag = (n[0]*vs[0]->m[0]) +
          (n[1]*vs[0]->m[1]) +
          (n[2]*vs[0]->m[2]);
    
    /* Assign */
    out->m[0] = (float)n[0];
    out->m[1] = (float)n[1];
    out->m[2] = (float)n[2];
    out->d    = (float)mag;

    return true;
}



bool
normal_from_face(
    float *out,
    vert const *verts,
    face *f,
    bool normalise)
{
    vert const *vs[3];
    double e0[3], e1[3], n[3], mag;
    
    if (!out || !verts || !f) return false;
    
    /* Faces wound clockwise are considered 'front facing' */
    vs[0] = verts + f->i[0];
    vs[1] = verts + f->i[2];
    vs[2] = verts + f->i[1];
    
    /* Calculate edges */
    e0[0] = (double)vs[1]->m[0] - (double)vs[0]->m[0];
    e0[1] = (double)vs[1]->m[1] - (double)vs[0]->m[1];
    e0[2] = (double)vs[1]->m[2] - (double)vs[0]->m[2];
    
    e1[0] = (double)vs[2]->m[0] - (double)vs[0]->m[0];
    e1[1] = (double)vs[2]->m[1] - (double)vs[0]->m[1];
    e1[2] = (double)vs[2]->m[2] - (double)vs[0]->m[2];

    /* Take the cross product */
    n[0] = (e0[1]*e1[2]) - (e0[2]*e1[1]);
    n[1] = (e0[2]*e1[0]) - (e0[0]*e1[2]);
    n[2] = (e0[0]*e1[1]) - (e0[1]*e1[0]);
    
    /* Normalise */
    if (normalise)
    {
        mag = (n[0]*n[0]) + (n[1]*n[1]) + (n[2]*n[2]);
        if (mag < DBL_EPSILON)
        {
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
            return false;
        }
        mag = sqrt(mag);
        n[0] /= mag;
        n[1] /= mag;
        n[2] /= mag;
    }
    
    /* Assign */
    out[0] = (float)n[0];
    out[1] = (float)n[1];
    out[2] = (float)n[2];
    
    return true;
}

