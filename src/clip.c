/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Defines subroutines related to (planar) clipping of geometry in order to
    facilitate compatibility with BSP partitioning.
*******************************************************************************/

#include "clip.h"
#include "draw.h"
#include "math.h"
#include "verbose.h"

#include <assert.h>
#include <stdio.h>
#include <signal.h>



extern unsigned   int clips;
extern unsigned short g_poly_clip, g_poly_inter, g_vert_012[3];
extern      DRAW_MODE g_draw_mode;
//extern           bool g_draw_clip;



static void
clip_draw(pools        *pool,
           face        *in,
          plane const  *clipper,
           vert const **vs)
{
    g_draw_mode = DRAW_MODE_CLIP;
    g_poly_clip = clipper - pool->planes;
    g_poly_inter = in - pool->faces;
    if (vs)
    {
        g_vert_012[0] = vs[0] - pool->verts;
        g_vert_012[1] = vs[1] - pool->verts;
        g_vert_012[2] = vs[2] - pool->verts;
    }
    else
    {
        g_vert_012[0] = 0xFFFF;
        g_vert_012[1] = 0xFFFF;
        g_vert_012[2] = 0xFFFF;
    }
    draw_pause();
    g_draw_mode = DRAW_MODE_UNSPECIFIED;
}



bool
clip_face(clip_pivot       *pivot,
               pools       *pool,
            unsigned short  face_i,
            unsigned short  clipper_i,
            unsigned short *advance)
{
    size_t nverts;
    
    face *in;
    plane *clipper;

    VERBOSE_2(float nm[2][3];)
    
    float e[3][3] = {};
    vert const *verts;
    vert const *v[3];
    vert const *swp_v;

    bool two = false;
    bool left_light = false;

    /* Param check */
    if (!pivot || !pool || !advance)
    {
        return false;
    }
    *advance = 0;
    
    /* Prepare buffers */
    if (!pools_vert_declare(pool, 2) ||
        !pools_face_declare(pool, 2))
    {
        fprintf(stderr, "Clipping error: Could not accommodate new polygons.\n");
        return false;
    }
    
    if (face_i >= pool->n_faces || clipper_i >= pool->n_faces)
    {
        VERBOSE_1
        (
            fprintf(stderr, "ERROR: clip_face() called with out-of-bounds face index!\n");
            raise(SIGINT);
        )
        return false;
    }

    /* Quick access */
    verts  = pool->verts;
    nverts = pool->n_verts;
    in     = pool->faces + face_i;
    clipper = pool->planes + clipper_i;
    
    if (in->i[0] >= nverts)
    {
        VERBOSE_1
        (
            fprintf(stderr, "ERROR: clip_face() f->v[0] out of bounds.\n");
            raise(SIGINT);
        )
        return false;
    }
    if (in->i[1] >= nverts)
    {
        VERBOSE_1
        (
            fprintf(stderr, "ERROR: clip_face() f->v[1] out of bounds.\n");
            raise(SIGINT);
        )
        return false;
    }
    if (in->i[2] >= nverts)
    {
        VERBOSE_1
        (
            fprintf(stderr, "ERROR: clip_face() f->v[2] out of bounds.\n");
            raise(SIGINT);
        )
        return false;
    }
    v[0] = verts + in->i[0];
    v[1] = verts + in->i[1];
    v[2] = verts + in->i[2];
    
    /* Derive normal */
    VERBOSE_2(normal_from_face(&nm[0][0], verts, in, false);)

    /* Determine which vertex is separated, if any */
    e[0][0] = vec3_distance_to_plane(clipper, v[0]->m);
    e[0][1] = vec3_distance_to_plane(clipper, v[1]->m);
    e[0][2] = vec3_distance_to_plane(clipper, v[2]->m);

#define VERT_ROT_CW\
    swp_v=v[0];\
    v[0]=v[2];\
    v[2]=v[1];\
    v[1]=swp_v;\
    e[1][0]=e[0][0];\
    e[0][0]=e[0][2];\
    e[0][2]=e[0][1];\
    e[0][1]=e[1][0]

#define VERT_ROT_CCW\
    swp_v=v[0];\
    v[0]=v[1];\
    v[1]=v[2];\
    v[2]=swp_v;\
    e[1][0]=e[0][0];\
    e[0][0]=e[0][1];\
    e[0][1]=e[0][2];\
    e[0][2]=e[1][0]

    /* Determine intersection type and reorder vertex pointers */
    if (e[0][0] <= -FLT_EPSILON) /* v0 < p */
    {
        VERBOSE_3(fprintf(stderr, "v0 behind clipper\n");)
        
        if (e[0][1] <= -FLT_EPSILON) /* v0 v1 < p */
        {
            two = true;

            VERBOSE_3(fprintf(stderr, "v1 behind clipper\n");)

            if (e[0][2] < FLT_EPSILON) /* v0 v1 < v2 <= p */
            {
                /* No intersection */
                VERBOSE_2
                (
                    fprintf(stderr, "v2 behind or touching clipper!\n");
                    fprintf(stderr, "Face behind clipper!\n");
                )
                return false;
            }
            else /* v0 v1 < p < v2 */
            {
                /* Rotate 2 into 0 (pivotal) */
                VERBOSE_3
                (
                    fprintf(stderr, "v2 ahead of clipper\n");
                    fprintf(stderr, "CW\n");
                )
                VERT_ROT_CW;
            }
        }
        else if (e[0][1] >= FLT_EPSILON) /* v0 < p < v1 */
        {
            VERBOSE_3(fprintf(stderr, "v1 ahead of clipper\n");)
        
            if (e[0][2] <= -FLT_EPSILON) /* v0 v2 < p < v1 */
            {
                two = true;

                VERBOSE_3
                (
                    fprintf(stderr, "v2 behind clipper\n");
                    fprintf(stderr, "CCW\n");
                )
                VERT_ROT_CCW;
            }
            else if (e[0][2] >= FLT_EPSILON) /* v0 < p < v1 v2 */
            {
                two = true;
                left_light = true;
                
                VERBOSE_3
                (
                    fprintf(stderr, "v2 ahead of clipper\n");
                    fprintf(stderr, "NO ROTATION\n");
                )

                /* PERFECT ORDERING HERE. No rotation necessary */
            }
            else /* v0 < (p == v2) < v1 */
            {
                two = false;
                left_light = true;
                
                VERBOSE_3
                (
                    fprintf(stderr, "v2 clips\n");
                    fprintf(stderr, "CW\n");
                )

                VERT_ROT_CW;
            }
        }
        else /* v0 < (p == v1) */
        {
            VERBOSE_3(fprintf(stderr, "v1 clips\n");)
            
            if (e[0][2] < FLT_EPSILON) /* v0 < (p == v1) <= v2 */
            {
                VERBOSE_3
                (
                    fprintf(stderr, "v2 behind or touching clipper!\n");
                    fprintf(stderr, "Face behind clipper!\n");
                )
                return false;
            }
            VERBOSE_3
            (
                fprintf(stderr, "v2 ahead of clipper\n");
                fprintf(stderr, "CCW\n");
            )
            
            two = false; /* v0 < (p == v1) < v2 */
            VERT_ROT_CCW;
        }
    }
    else if (e[0][0] >= FLT_EPSILON) /* p < v0 */
    {
        VERBOSE_3(fprintf(stderr, "v0 ahead of clipper\n");)

        if (e[0][1] >= FLT_EPSILON) /* p < v0 v1 */
        {
            two = true;

            VERBOSE_3(fprintf(stderr, "v1 ahead of clipper\n");)

            if (e[0][2] > -FLT_EPSILON) /* p < v0 v1 v2 */
            {
                /* No intersection */
                VERBOSE_2
                (
                    fprintf(stderr, "v2 ahead of or touching clipper!\n");
                    fprintf(stderr, "Face ahead of clipper!\n");
                )
                return false;
            }
            else /* v2 < p < v0 v1 */
            {
                left_light = true;
                
                VERBOSE_3
                (
                    fprintf(stderr, "v2 behind clipper\n");
                    fprintf(stderr, "CW\n");
                )
                
                /* Rotate 2 into 0 (pivotal) */
                VERT_ROT_CW;
            }
        }
        else if (e[0][1] <= -FLT_EPSILON) /* v1 < p < v0 */
        {
            VERBOSE_3(fprintf(stderr, "v1 behind clipper\n");)
            
            if (e[0][2] <= -FLT_EPSILON) /* v1 v2 < p < v0 */
            {
                two = true;
                
                VERBOSE_3
                (
                    fprintf(stderr, "v2 behind clipper\n");
                    fprintf(stderr, "NO ROTATION\n");
                )

                /* PERFECT ORDERING HERE. No rotation necessary */
            }
            else if (e[0][2] >= FLT_EPSILON) /* v1 < p < v0 v2 */
            {
                two = true;
                left_light = true;

                VERBOSE_3
                (
                    fprintf(stderr, "v2 ahead of clipper\n");
                    fprintf(stderr, "CCW\n");
                )

                VERT_ROT_CCW;
            }
            else /* v1 < (p == v2) < v0 */
            {
                two = false;

                VERBOSE_3
                (
                    fprintf(stderr, "v2 clips\n");
                    fprintf(stderr, "CW\n");
                )

                VERT_ROT_CW;
            }
        }
        else /* (p == v1) < v0 */
        {
            VERBOSE_3(fprintf(stderr, "v1 clips\n");)
            
            if (e[0][2] > -FLT_EPSILON) /* (p == v1) <= v2 < v0 */
            {
                VERBOSE_2
                (
                    fprintf(stderr, "v2 ahead of or touching clipper!\n");
                    fprintf(stderr, "Face ahead of clipper!\n");
                )
                return false;
            }
            
            VERBOSE_3
            (
                fprintf(stderr, "v2 behind clipper\n");
                fprintf(stderr, "CCW\n");
            )
            two = false; /* v0 < (p == v1) < v2 */
            left_light = true;
            VERT_ROT_CCW;
        }
    }
    else /* (p == v0) */
    {
        VERBOSE_3(fprintf(stderr, "v0 clips\n");)
        
        two = false;
        if (e[0][1] <= -FLT_EPSILON)
        {
            VERBOSE_3(fprintf(stderr, "v1 behind clipper\n");)
            
            if (e[0][2] < FLT_EPSILON)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "v2 behind or touching clipper!\n");
                    fprintf(stderr, "Face behind clipper!\n");
                )
                
                return false;
            }
            
            VERBOSE_3(fprintf(stderr, "v2 ahead of clipper\n");)
            left_light = true;
        }
        else if(e[0][1] >= FLT_EPSILON)
        {
            VERBOSE_3(fprintf(stderr, "v1 ahead of clipper\n");)
            
            if (e[0][2] > -FLT_EPSILON)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "v2 ahead of or touching clipper!\n");
                    fprintf(stderr, "Face ahead of clipper!\n");
                )

                return false;
            }
        }
        else
        {
            VERBOSE_2(fprintf(stderr, "v1 also touching clipper "
                                      "(no intersection)\n");)

            return false;
        }
    }
#undef VERT_ROT

    clip_draw(pool, in, clipper, v);

    /* Derive normal */
#if VERBOSE >= 2
    normal_from_face(&nm[1][0], verts, in, false);
    
    if (vec3_dot(&nm[0][0], &nm[1][0]) < FLT_EPSILON)
    {
        fprintf(stderr, "ERROR: After rotation, the face faces opposite!\n");
    }
#endif

#if VERBOSE >= 2
    if (two)
    {
        if (left_light)
        {
            if( e[0][0] <= -FLT_EPSILON &&
                e[0][1] >=  FLT_EPSILON && e[0][2] >=  FLT_EPSILON ) {}
            else
            {
                fprintf(stderr, "Assertion failure (two && left_light).\n");
                raise(SIGINT);
                return false;
            }
        }
        else
        {
            if( e[0][0] >=  FLT_EPSILON &&
                e[0][1] <= -FLT_EPSILON && e[0][2] <= -FLT_EPSILON ) {}
            else
            {
                fprintf(stderr, "Assertion failure (two && !left_light).\n");
                raise(SIGINT);
                return false;
            }
        }
    }
    else
    {
        if (fabs(e[0][0]) < FLT_EPSILON) {}
        else
        {
            fprintf(stderr, "Assertion failure (!two && !clipping epsilon).\n");
            raise(SIGINT);
            return false;
        }
        if (left_light)
        {
            if (   e[0][1] <= -FLT_EPSILON &&
                   e[0][2] >=  FLT_EPSILON) {}
            else
            {
                fprintf(stderr, "Assertion failure (!two && left_light).\n");
                raise(SIGINT);
                return false;
            }
        }
        else
        {
            if (   e[0][1] >=  FLT_EPSILON &&
                   e[0][2] <= -FLT_EPSILON) {}
            else
            {
                fprintf(stderr, "Assertion failure (!two && !left_light).\n");
                raise(SIGINT);
                return false;
            }
        }
    }
#endif

    /* Are we splitting one or two edges? */
    if (two)
    {
        face nf;
        unsigned short ev0, ev1; /* Edge verts */
        
        VERBOSE_2
        (
            fprintf(stderr, "Split in three.\n");
        )
        
        /* Split edges */
        if (!vec3_plane_ray_intersect(clipper, &e[1][0], v[0]->m, v[1]->m))
        {
            fprintf(stderr, "First edge clipping failed.\n");
            VERBOSE_1(raise(SIGINT);)
            return false;
        }
        if (!vec3_plane_ray_intersect(clipper, &e[2][0], v[0]->m, v[2]->m))
        {
            fprintf(stderr, "Second edge clipping failed.\n");
            VERBOSE_1(raise(SIGINT);)
            return false;
        }
        
        ev0 = pools_vert_decl_add_f(pool, &e[1][0]);
        ev1 = pools_vert_decl_add_f(pool, &e[2][0]);
        
        if (0xFFFF == ev0 || 0xFFFF == ev1)
        {
            fprintf(stderr,
                    "Failure allocating additional vertices in clipping.\n");
            return false;
        }
        
        if (face_i > pivot->pr)
        {
            /* Replacement face is on the right of the pivot */
            if (left_light)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on right, left light.\n");
                )
                
                /* One face from the clip right can replace it */
                in->i[0] = ev0;
                in->i[1] = v[1] - verts;
                in->i[2] = v[2] - verts;
                
                /* Insert remaining faces */
                nf.i[0] = v[2] - verts; // Right
                nf.i[1] = ev1;
                nf.i[2] = ev0;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            face_i+1))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->r;
                ++ (*advance);
                
                nf.i[0] = v[0] - verts; // Left
                nf.i[1] = ev0;
                nf.i[2] = ev1;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pl))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->pl;
                ++ pivot->pr;
                ++ pivot->r;
                ++ (*advance);
            }
            else
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on right, left heavy.\n");
                )
                
                /* The face on the clip right replaces it */
                in->i[0] = v[0] - verts;
                in->i[1] = ev0;
                in->i[2] = ev1;
                
                /* Populate remaining faces */
                nf.i[0] = ev0;
                nf.i[1] = v[1] - verts;
                nf.i[2] = v[2] - verts;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pl))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->pl;
                ++ pivot->pr;
                ++ pivot->r;
                ++ (*advance);
                
                nf.i[0] = v[2] - verts;
                nf.i[1] = ev1;
                nf.i[2] = ev0;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pl))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->pl;
                ++ pivot->pr;
                ++ pivot->r;
                ++ (*advance);
            }
        }
        else if (face_i < pivot->pl)
        {
            /* Replacement face is on the left of the pivot */
            if (left_light)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on left, left light.\n");
                )
                
                /* The face on the clip left replaces it */
                in->i[0] = v[0] - verts;
                in->i[1] = ev0;
                in->i[2] = ev1;
                
                /* Populate remaining faces */
                nf.i[0] = ev0;
                nf.i[1] = v[1] - verts;
                nf.i[2] = v[2] - verts;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pr+1))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->r;
                
                nf.i[0] = v[2] - verts;
                nf.i[1] = ev1;
                nf.i[2] = ev0;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pr+1))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->r;
            }
            else
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on left, left heavy.\n");
                )
                
                /* One face on the clip left can replace it */
                in->i[0] = ev0;
                in->i[1] = v[1] - verts;
                in->i[2] = v[2] - verts;
                
                /* Insert remaining faces */
                nf.i[0] = v[0] - verts; // Right
                nf.i[1] = ev0;
                nf.i[2] = ev1;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pr+1))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot->r;
                
                nf.i[0] = v[2] - verts; // Left
                nf.i[1] = ev1;
                nf.i[2] = ev0;
                if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                            pivot->pl))
                {
                    goto L_FailInsertFace;
                }
                ++ clips;
                ++ pivot-> r;
                ++ pivot->pl;
                ++ pivot->pr;
            }
        }
        else
        {
            fprintf(stderr, "BUG: Somehow, a pivot triangle was sent for "
                            "clipping (i = %hu).\n", face_i);
            VERBOSE_1(raise(SIGINT);)
        }
    }
    else /* One edge */
    {
        face nf;
        unsigned short ev; /* Edge vertex */
        
        VERBOSE_2
        (
            fprintf(stderr, "Split in two.\n");
        )
        
        /* Split edge */
        if (!vec3_plane_ray_intersect(clipper, &e[1][0], v[1]->m, v[2]->m))
        {
            fprintf(stderr, "Edge clipping failed.\n");
            VERBOSE_1(raise(SIGINT);)
            return false;
        }
        
        ev = pools_vert_decl_add_f(pool, &e[1][0]);
        
        if (ev == 0xFFFF)
        {
            fprintf(stderr, "Failure allocating additional vertices in "
                            "clipping.\n");
            return false;
        }
        
        if (face_i > pivot->pr)
        {
            /* The face is originally on the right of the pivot.
             * Replace the face with the right clip */
            if (left_light)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on right, left light.\n");
                )
                
                /* Right face gets replaced */
                in->i[0] = v[0] - verts;
                in->i[1] = ev;
                in->i[2] = v[2] - verts;
                
                /* Add new face on left (of pivot) */
                nf.i[0] = v[0] - verts;
                nf.i[1] = v[1] - verts;
                nf.i[2] = ev;
            }
            else
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on right, left heavy.\n");
                )
                
                /* Right face gets replaced */
                in->i[0] = v[0] - verts;
                in->i[1] = v[1] - verts;
                in->i[2] = ev;
                
                /* Add new face on left */
                nf.i[0] = v[0] - verts;
                nf.i[1] = ev;
                nf.i[2] = v[2] - verts;
            }
            
            /* Insert face into respective position */
            if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                        pivot->pl))
            {
                goto L_FailInsertFace;
            }
            ++ clips;
            ++ pivot->pl;
            ++ pivot->pr;
            ++ pivot-> r;
            ++ (*advance);
        }
        else if (face_i < pivot->pl)
        {
            /* The face is originally on the left of the pivot.
             * Replace the face with the left clip */
            if (left_light)
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on left, left light.\n");
                )
                
                /* Left face gets replaced */
                in->i[0] = v[0] - verts;
                in->i[1] = v[1] - verts;
                in->i[2] = ev;
                
                /* Add new face on left */
                nf.i[0] = v[0] - verts;
                nf.i[1] = ev;
                nf.i[2] = v[2] - verts;
            }
            else
            {
                VERBOSE_2
                (
                    fprintf(stderr, "Pivot on left, left heavy.\n");
                )
                
                /* Left face gets replaced */
                in->i[0] = v[0] - verts;
                in->i[1] = ev;
                in->i[2] = v[2] - verts;
                
                /* Add new face on left */
                nf.i[0] = v[0] - verts;
                nf.i[1] = v[1] - verts;
                nf.i[2] = ev;
            }
            
            /* Insert face into respective position */
            if (!pools_face_decl_insert(pool, &nf, pool->planes + face_i,
                                        pivot->pr+1))
            {
                goto L_FailInsertFace;
            }
            ++ clips;
            ++ pivot->r;
        }
        else
        {
            fprintf(stderr, "BUG: Somehow, a pivot triangle was sent for "
                            "clipping (i = %hu).\n", face_i);
            VERBOSE_1(raise(SIGINT);)
        }
    }

    return true;
    
    
L_FailInsertFace:
    fprintf(stderr, "Failure allocating additional faces in clipping.\n");
    return false;
}



#if 0
bool clip_append(clipping const *const clip,
                    pools const *const pool,
           unsigned short              pivot)
{
    /* TODO */
}
#endif



#if 0
bool
clip_get_verts(clipping const  *const clip,
                  pools        *const pool,
                   vert       **      out,
         unsigned  char               fi)
{
    if (clip && pool && out)
    {
        unsigned short vbit, vsrc;
        bool const two = clip->two;
        unsigned char const fe = two ? 3 : 2;

        if (fi < fe)
        {
            vert       *const verts0 = clip->clip;
            face const *const face   = clip->faces + fi;
            vert       *const verts1 = pool->verts;

            vsrc = clip->vsrc;
            vbit = 1<<(fi*3);

            out[0] = (( vbit     & vsrc) ? verts0 : verts1) + face->i[0];
            out[1] = (((vbit<<1) & vsrc) ? verts0 : verts1) + face->i[1];
            out[2] = (((vbit<<2) & vsrc) ? verts0 : verts1) + face->i[2];

            return true;
        }
    }

    return false;
}
#endif
