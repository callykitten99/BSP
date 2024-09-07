#ifndef MATH_H
#define MATH_H

#include "vert.h"
#include "face.h"
#include "plane.h"

#include <stdbool.h>
#include <math.h>
#include <float.h>


bool plane_from_face(
    plane *out,
    vert const *verts,
    face *f);

bool normal_from_face(
    float *out,
    vert const *verts,
    face *f,
    bool normalise);


static inline void
vec3_add(float *out,
         float const *a, float const *b)
{
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

static inline void
vec3_sub(float *out,
         float const *a, float const *b)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline bool
vec3_norm(float *out,
          float const *a)
{
    double const mag2 = (a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    if (mag2 >= DBL_EPSILON) {
        double const mag = sqrt(mag2);
        out[0] = (float)(a[0] / mag);
        out[1] = (float)(a[1] / mag);
        out[2] = (float)(a[2] / mag);
        return true;
    } return false;
}

static inline void
vec3_cross(float *out,
           float const *a, float const *b)
{
    out[0] = (a[1]*b[2]) - (a[2]*b[1]);
    out[1] = (a[2]*b[0]) - (a[0]*b[2]);
    out[2] = (a[0]*b[1]) - (a[1]*b[0]);
}

static inline float
vec3_dot(float const *a,
         float const *b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/*static inline float 
vec3_distance_to_plane(plane const *p,
                       float const *a)
{
    return vec3_dot(p->m, a) - p->d;
}*/
static inline float 
vec3_distance_to_plane(plane const *p,
                       float const *a)
{
    return (float)
        (((double)a[0] * (double)p->m[0]  +
          (double)a[1] * (double)p->m[1]  +
          (double)a[2] * (double)p->m[2]) -
          (double)p->d);
}



static inline void
vec3_project_plane(plane const *p,
                   float       *out,
                   float const *in)
{
    /* Dot product */
    double const dist =
        ((double)p->m[0] * (double)in[0]) +
        ((double)p->m[1] * (double)in[1]) +
        ((double)p->m[2] * (double)in[2]) -
        (double)p->d;

    out[0] = (float)((double)in[0] - (dist * (double)p->m[0]));
    out[1] = (float)((double)in[1] - (dist * (double)p->m[1]));
    out[2] = (float)((double)in[2] - (dist * (double)p->m[2]));
}



static inline float
vec3_project_plane_get_d(plane const *p,
                         float       *out,
                         float const *in)
{
    /* Dot product */
    double const dist =
        ((double)p->m[0] * (double)in[0]) +
        ((double)p->m[1] * (double)in[1]) +
        ((double)p->m[2] * (double)in[2]) -
         (double)p->d;

    out[0] = (float)((double)in[0] - (dist * (double)p->m[0]));
    out[1] = (float)((double)in[1] - (dist * (double)p->m[1]));
    out[2] = (float)((double)in[2] - (dist * (double)p->m[2]));

    return (float)dist;
}



static inline bool
vec3_plane_ray_intersect(plane const *p,
                         float       *out,
                         float const *l0,
                         float const *l1)
{
    /* Derive line direction vector */
    double const l[3] =
    {
        (double)l1[0] - (double)l0[0],
        (double)l1[1] - (double)l0[1],
        (double)l1[2] - (double)l0[2],
    };
    
    /* Dot with normal (discriminant) */
    double const ldN =
        l[0] * (double)p->m[0] +
        l[1] * (double)p->m[1] +
        l[2] * (double)p->m[2];
    if (fabs(ldN) >= DBL_EPSILON)
    {
        /* Dot l0 with normal */
        double const l0dN = (double)p->d -
           (l0[0] * (double)p->m[0] +
            l0[1] * (double)p->m[1] +
            l0[2] * (double)p->m[2]);
        
        /* Determine interpolation value */
        double const a = l0dN / ldN;
        
        /* Return */
        out[0] = (double)l0[0] + a*l[0];
        out[1] = (double)l0[1] + a*l[1];
        out[2] = (double)l0[2] + a*l[2];
        return true;
    }
    return false;
}



static inline void mat4_transpose(float *mat)
{
#define MAT4_SWAP(i,j)\
    swap=mat[i];\
    mat[i]=mat[j];\
    mat[j]=swap

    float swap;
    
    MAT4_SWAP(1,4);
    MAT4_SWAP(2,8);
    MAT4_SWAP(6,9);
    MAT4_SWAP(3,12);
    MAT4_SWAP(7,13);
    MAT4_SWAP(11,14);

#undef MAT4_SWAP
}

#endif /* MATH_H */
