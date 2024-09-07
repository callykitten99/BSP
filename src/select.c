#include "clip.h"
#include "draw.h"
#include "math.h"
#include "bsp.h"
#include "select.h"
#include "verbose.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SELF pools *const self

#ifndef PLANE_REL_DEF
#define PLANE_REL_DEF
typedef enum {
    PLANE_REL_LEFT = 0,
    PLANE_REL_RIGHT,
    PLANE_REL_INTER,
    PLANE_REL_COINCIDE,
} PLANE_REL;
#endif



extern unsigned short g_bsp_l, g_pivot_l, g_pivot_r, g_bsp_r;
extern bsp g_bsp;
extern DRAW_MODE g_draw_mode;

static unsigned int swaps  = 0U;
static unsigned int rdepth = 0U;
       unsigned int clips  = 0U;



void
select_get_props(SELF,
                 unsigned short *ints_out,
                 unsigned short *bal_out,
                 unsigned short l,
                 unsigned short fi,
                 unsigned short r)
{
    plane *p = self->planes + fi;
    face  *f = self->faces;
    vert  *v = self->verts;

    unsigned short ints = 0, i;
               int  bal = 0;

    /* Iterate per faces */
    for (i = l; i < r; ++i)
    {
        bool pos = false, neg = false;
        float d;

        /* Derive the vertex distances of each face from the plane */
        d = vec3_distance_to_plane(p, v[f[i].i[0]].m);
             if (d <= -FLT_EPSILON) neg = true;
        else if (d >=  FLT_EPSILON) pos = true;

        d = vec3_distance_to_plane(p, v[f[i].i[1]].m);
             if (d <= -FLT_EPSILON) neg = true;
        else if (d >=  FLT_EPSILON) pos = true;

        if (neg && pos) {
            ++ints;
            self->planes[i].rel = PLANE_REL_INTER;
            continue;
        }

        d = vec3_distance_to_plane(p, v[f[i].i[2]].m);
             if (d <= -FLT_EPSILON) neg = true;
        else if (d >=  FLT_EPSILON) pos = true;

        if (neg) {
            if (pos) {
                ++ints;
                self->planes[i].rel = PLANE_REL_INTER;
            } else {
                -- bal;
                self->planes[i].rel = PLANE_REL_LEFT;
            }
        } else {
            if (pos) {
                ++ bal;
                self->planes[i].rel = PLANE_REL_RIGHT;
            } else {
                self->planes[i].rel = PLANE_REL_COINCIDE;
            }
        }
    }

    *ints_out = ints;
    *bal_out  = (unsigned short)abs(bal);
}



PLANE_REL
select_rel(plane *p, face *f, vert *verts)
{
    /* Determine relationship */
    bool pos = false, neg = false;
    float d;
    
#if VERBOSE >= 2
    if (!p)
    {
        fprintf(stderr, "WARNING: select_rel(p = NULL)\n");
        return PLANE_REL_COINCIDE;
    }
    if (!f)
    {
        fprintf(stderr, "WARNING: select_rel(f = NULL)\n");
        return PLANE_REL_COINCIDE;
    }
    if (!verts)
    {
        fprintf(stderr, "WARNING: select_rel(verts = NULL)\n");
        return PLANE_REL_COINCIDE;
    }
#endif

    /* Derive the vertex distances of each face from the plane */
    d = vec3_distance_to_plane(p, verts[f->i[0]].m);
         if (d <= -FLT_EPSILON) neg = true;
    else if (d >=  FLT_EPSILON) pos = true;

    d = vec3_distance_to_plane(p, verts[f->i[1]].m);
         if (d <= -FLT_EPSILON) neg = true;
    else if (d >=  FLT_EPSILON) pos = true;

    /* Early intersection test opportunity */
    if (neg && pos)
        return PLANE_REL_INTER;

    /* Test final vertex */
    d = vec3_distance_to_plane(p, verts[f->i[2]].m);
         if (d <= -FLT_EPSILON) neg = true;
    else if (d >=  FLT_EPSILON) pos = true;

    /* Summarise */
    if (neg) {
        return pos ? PLANE_REL_INTER : PLANE_REL_LEFT;
    } else {
        return pos ? PLANE_REL_RIGHT : PLANE_REL_COINCIDE;
    }
}



void
select_move_left_simple(SELF,
                        unsigned short pivot,
                        unsigned short elem)
{
    face f_swp;
    plane p_swp;


#if VERBOSE >= 2
    /* Run checks */
    if (!self)
    {
        fprintf(stderr, "ERROR: select_move_left_simple(self = NULL)\n");
        return;
    }
    if (elem <= pivot)
    {
        fprintf(stderr, "ERROR: select_move_left_simple() "
                        "elem is already on left.\n");
        return;
    }
    if ((size_t)(pivot+1) >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_left_simple() pivot boundary.\n");
        return;
    }
    if (elem >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_left_simple() elem boundary.\n");
        return;
    }
#endif


    VERBOSE_3(fprintf(stderr, "select_move_left(%hu across %hu)\n",
                              elem, pivot);)


    ++swaps;
    
    /* First take out pivot */
    f_swp = self->faces[pivot];
    p_swp = self->planes[pivot];

    /* Put element in place of pivot */
    self->faces[pivot]  = self->faces[elem];
    self->planes[pivot] = self->planes[elem];

    /* If the element to move is not the pivot's neighbour */
    if (pivot+1 != elem) {
        /* Put pivot neighbour into old position of element */
        self->faces[elem]  = self->faces[pivot+1];
        self->planes[elem] = self->planes[pivot+1];

        /* Replace pivot, into neighbour's position */
        self->faces [pivot+1] = f_swp;
        self->planes[pivot+1] = p_swp;
    } else {
        /* Replace pivot, into old position of element */
        self->faces [elem] = f_swp;
        self->planes[elem] = p_swp;
    }
}

void
select_move_right_simple(SELF,
                         unsigned short pivot,
                         unsigned short elem)
{
    face f_swp;
    plane p_swp;


#if VERBOSE >= 2
    /* Run checks */
    if (!self)
    {
        fprintf(stderr, "ERROR: select_move_right_simple(self = NULL)\n");
        return;
    }
    if (elem >= pivot)
    {
        fprintf(stderr, "ERROR: select_move_right_simple() "
                        "elem is already on right.\n");
        return;
    }
    if ((size_t)(pivot-1) >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_right_simple() pivot boundary.\n");
        return;
    }
    if (elem >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_right_simple() elem boundary.\n");
        return;
    }
#endif


    VERBOSE_3(fprintf(stderr, "select_move_right(%hu across %hu)\n",
                              elem, pivot);)


    ++swaps;
    
    /* First take out pivot */
    f_swp = self->faces[pivot];
    p_swp = self->planes[pivot];

    /* Put element in place of pivot */
    self->faces [pivot] = self->faces[elem];
    self->planes[pivot] = self->planes[elem];

    /* If the element to move is not the pivot's neighbour */
    if (pivot-1 != elem) {
        /* Put pivot neighbour into old position of element */
        self->faces [elem] = self->faces [pivot-1];
        self->planes[elem] = self->planes[pivot-1];

        /* Replace pivot, into neighbour's position */
        self->faces [pivot-1] = f_swp;
        self->planes[pivot-1] = p_swp;
    } else {
        /* Replace pivot, into old position of element */
        self->faces [elem] = f_swp;
        self->planes[elem] = p_swp;
    }
}

void
select_move_left(SELF,
                 unsigned short pivot_l,
                 unsigned short pivot_r,
                 unsigned short elem)
{
    face f_swp;
    plane p_swp;


#if VERBOSE >= 2
    if (!self)
    {
        fprintf(stderr, "ERROR: select_move_left(self = NULL)\n");
        return;
    }
#endif
    
    if (pivot_l == pivot_r)
        return select_move_left_simple(self, pivot_l, elem);
    
    if (pivot_l > pivot_r)
    {
        unsigned short temp = pivot_l;
        pivot_l = pivot_r;
        pivot_r = temp;
        
        VERBOSE_2
        (
        fprintf(stderr, "ERROR (non-fatal): select_move_left() ranged pivot not"
                        " in proper order.\n");
        )
    }
    
#if VERBOSE >= 2
    if (elem <= pivot_r)
    {
        fprintf(stderr, "ERROR: select_move_left() elem is already on left.\n");
        return;
    }
    if ((size_t)(pivot_r+1) >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_left() pivot boundary.\n");
        return;
    }
    if (elem >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_left() elem boundary.\n");
        return;
    }
#endif


    VERBOSE_3(fprintf(stderr, "select_move_left(%hu across %hu -> %hu)\n",
                              elem, pivot_l, pivot_r);)


    ++swaps;
     
    /* First take out neighbour */
    f_swp = self->faces [pivot_r+1];
    p_swp = self->planes[pivot_r+1];
    
    /* Move pivot */
    memmove(&self->faces[pivot_l+1], &self->faces[pivot_l],
            (1+pivot_r-pivot_l) * sizeof(face));
    memmove(&self->planes[pivot_l+1], &self->planes[pivot_l],
            (1+pivot_r-pivot_l) * sizeof(plane));

    /* If the element to move is not the pivot's neighbour */
    if (pivot_r+1 != elem) {
        /* Move element into position */
        self->faces [pivot_l] = self->faces [elem];
        self->planes[pivot_l] = self->planes[elem];

        /* Replace neighbour */
        self->faces [elem] = f_swp;
        self->planes[elem] = p_swp;
    } else {
        /* Move element into position */
        self->faces [pivot_l] = f_swp;
        self->planes[pivot_l] = p_swp;
    }
}

void
select_move_right(SELF,
                  unsigned short pivot_l,
                  unsigned short pivot_r,
                  unsigned short elem)
{
    face f_swp;
    plane p_swp;


#if VERBOSE >= 2
    if (!self)
    {
        fprintf(stderr, "ERROR: select_move_right(self = NULL)\n");
        return;
    }
#endif
    
    if (pivot_l == pivot_r)
        return select_move_right_simple(self, pivot_l, elem);
    
    if (pivot_l > pivot_r)
    {
        unsigned short temp = pivot_l;
        pivot_l = pivot_r;
        pivot_r = temp;
        
        VERBOSE_2
        (
        fprintf(stderr, "ERROR (non-fatal): select_move_right() ranged pivot not"
                        " in proper order.\n");
        )
    }
    
#if VERBOSE >= 2
    if (elem >= pivot_l)
    {
        fprintf(stderr, "ERROR: select_move_right() elem is already on right.\n");
        return;
    }
    if ((size_t)(pivot_l-1) >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_right() pivot boundary.\n");
        return;
    }
    if (elem >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_right() elem boundary.\n");
        return;
    }
#endif


    VERBOSE_3(fprintf(stderr, "select_move_right(%hu across %hu -> %hu)\n",
                              elem, pivot_l, pivot_r);)


    ++swaps;
    
    /* First take out neighbour */
    f_swp = self->faces [pivot_l-1];
    p_swp = self->planes[pivot_l-1];
    
    /* Move pivot */
    memmove(&self->faces[pivot_l-1], &self->faces[pivot_l],
            (1+pivot_r-pivot_l) * sizeof(face));
    memmove(&self->planes[pivot_l-1], &self->planes[pivot_l],
            (1+pivot_r-pivot_l) * sizeof(plane));

    /* If the element to move is not the pivot's neighbour */
    if (pivot_l-1 != elem) {
        /* Put pivot neighbour into old position of element */
        self->faces [pivot_r] = self->faces [elem];
        self->planes[pivot_r] = self->planes[elem];

        /* Replace pivot, into neighbour's position */
        self->faces [elem] = f_swp;
        self->planes[elem] = p_swp;
    } else {
        /* Put element into new spot */
        self->faces [pivot_r] = f_swp;
        self->planes[pivot_r] = p_swp;
    }
}

void select_move_coincident(SELF,
                            unsigned short pivot_l,
                            unsigned short pivot_r,
                            unsigned short elem)
{
    face  f_swp;
    plane p_swp;


#if VERBOSE >= 2
    if (!self)
    {
        fprintf(stderr, "ERROR: select_move_coincident(self = NULL)\n");
        return;
    }
    if (elem >= self->n_faces)
    {
        fprintf(stderr, "ERROR: select_move_coincident() elem boundary.\n");
        return;
    }
    if (pivot_l > pivot_r)
    {
        unsigned short swap;
        swap = pivot_l;
        pivot_l = pivot_r;
        pivot_r = swap;
    }
#endif


    VERBOSE_3(fprintf(stderr, "select_move_coincident(%hu joins %hu -> %hu)\n",
                              elem, pivot_l, pivot_r);)


    ++swaps;

    if (elem < pivot_l)
    {
        if ((size_t)(pivot_l-1) >= self->n_faces)
        {
            VERBOSE_2
            (
                fprintf(stderr, "ERROR: select_move_coincident() pivot "
                                "boundary.\n");
            )
            return;
        }

        /* Move neighbour into swap */
        f_swp = self->faces [pivot_l-1];
        p_swp = self->planes[pivot_l-1];
        
        /* Move pivot */
        memmove(&self->faces[pivot_l-1], &self->faces[pivot_l],
                (1+pivot_r-pivot_l) * sizeof(face));
        memmove(&self->planes[pivot_l-1], &self->planes[pivot_l],
                (1+pivot_r-pivot_l) * sizeof(plane));
        
        if (elem == pivot_l-1)
        {
            self->faces [pivot_r] = self->faces [elem];
            self->planes[pivot_r] = self->planes[elem];
        }
        else
        {
            /* Move element ahead of pivot */
            self->faces [pivot_r] = self->faces [elem];
            self->planes[pivot_r] = self->planes[elem];
            
            /* Move swap into element */
            self->faces [elem] = f_swp;
            self->planes[elem] = p_swp;
        }
    }
    else if (elem > pivot_r)
    {
        if ((size_t)(pivot_r+1) >= self->n_faces)
        {
            VERBOSE_2
            (
                fprintf(stderr, "ERROR: select_move_coincident() pivot "
                                "boundary.\n");
            )
            return;
        }

        if (elem == pivot_r+1) return;

        f_swp = self->faces [pivot_r+1];
        p_swp = self->planes[pivot_r+1];

        self->faces [pivot_r+1] = self->faces [elem];
        self->planes[pivot_r+1] = self->planes[elem];

        self->faces [elem] = f_swp;
        self->planes[elem] = p_swp;
    }
    else
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: Attempt to make pivot coincident.\n");
        )
    }
}



void draw_set_clip(clip_pivot const *pv)
{
    if (pv)
    {
        g_bsp_l = pv->l;
        g_bsp_r = pv->r;
        g_pivot_l = pv->pl;
        g_pivot_r = pv->pr;
    }
    else
    {
        g_bsp_l = 0;
        g_bsp_r = -1;
        g_pivot_l = 0;
        g_pivot_r = 0;
    }
}



/* Returns position of pivot after partitioning */
bsp_ind
select_partition(SELF, clip_pivot *pivot)
{
    clip_pivot cp = *pivot;

    unsigned short i, inc;


    /* Move anything left of pivot to the right if applicable */
    for (i = cp.l; i < cp.pl; ++i)
    {
        VERBOSE_3
        (
            draw_set_clip(pivot);
            draw_pause();
        )
        
        switch (select_rel(self->planes + cp.pl, self->faces + i, self->verts))
        {
        case PLANE_REL_LEFT:
            // As intended
            break;

        case PLANE_REL_RIGHT:
            select_move_right(self, cp.pl, cp.pr, i);
            --cp.pl; --cp.pr; --i;
            break;

        case PLANE_REL_INTER:
        #ifndef NO_CLIPPING
            clip_face(&cp, self, i, cp.pl, &inc);
            i += inc;
        #else
            select_move_coincident(self, cp.pl, cp.pr, i);
            --cp.pl; --i;
        #endif
            break;

        case PLANE_REL_COINCIDE:
            select_move_coincident(self, cp.pl, cp.pr, i);
            --cp.pl; --i;
            break;
        }
        
        VERBOSE_3
        (
            draw_set_clip(pivot);
            draw_pause();
        )
    }

    /* Move anything right of the pivot to the left if applicable */
    for (i = cp.pr+1; i < cp.r; ++i)
    {
        VERBOSE_3
        (
            draw_set_clip(pivot);
            draw_pause();
        )
        
        switch (select_rel(self->planes + cp.pl, self->faces + i, self->verts))
        {
        case PLANE_REL_LEFT:
            select_move_left(self, cp.pl, cp.pr, i);
            ++cp.pl; ++cp.pr;
            if (cp.pr+1 < i) --i;
            break;

        case PLANE_REL_RIGHT:
            // As intended
            break;

        case PLANE_REL_INTER:
        #ifndef NO_CLIPPING
            clip_face(&cp, self, i, cp.pl, &inc);
            i += inc;
        #else
            select_move_coincident(self, cp.pl, cp.pr, i);
            ++cp.pr;
        #endif
            break;

        case PLANE_REL_COINCIDE:
            select_move_coincident(self, cp.pl, cp.pr, i);
            ++cp.pr;
            if (cp.pr+1 < i) --i;
            break;
        }
        
        VERBOSE_3
        (
            draw_set_clip(pivot);
            draw_pause();
        )
    }
    
    /* Output new clipping pivot */
    *pivot = cp;

    /* Allocate and return a BSP node */
    return bsp_new(&g_bsp, cp.pl, cp.pr);
}



/*void
print_depth(unsigned int depth)
{
    depth <<= 1;
    while (depth--) fputc(' ', stderr);
}*/

bsp_ind
select_iter(SELF,
            clip_pivot *pivot,
            unsigned int depth)
{
    /* PARAMS MUST BE VERIFIED FIRST!
     * This function should only be called from select_begin() or recursively,
     * which checks parameters before iterating. */

    /* best = index of best polygon (so far)
     *    i = candidate index (comparator)
     *  bal = balance = abs(polys on left - polys on right)
     *   in = number of intersections through the polygon's plane */
    unsigned short best, i, bal, in, score;
    bsp_ind id;
    
    clip_pivot cp = *pivot;
    clip_pivot cparg;



    /* Only param check administered */
    if (cp.l >= cp.r) return 0xFFFF;
    
    /* Recursion depth check */
    if (depth > rdepth)
        rdepth = depth;
    


    VERBOSE_3
    (
        fprintf(stderr, "select_iter(");
    )

    /* Select initial pivot polygon for BSP */
    best = cp.l;
    select_get_props(self, &in, &bal, cp.l, cp.l, cp.r);
    score = bal + (in<<3);
    
    VERBOSE_3
    (
        g_draw_mode = DRAW_MODE_REL;
        draw_set_clip(&cp);
        draw_pause();
    )

    for (i = cp.l+1; i < cp.r; ++i)
    {
        /* Candidate balances and intersection counts */
        unsigned short balc, inc, scorec;

        select_get_props(self, &balc, &inc, cp.l, i, cp.r);

        /* Determine whether this pivot is better than the current best */
        scorec = balc + (inc<<3);
        if (scorec < score || (scorec == score && inc < in))
        {
            best  = i;
            bal   = balc;
            in    = inc;
            score = scorec;

            /* A score of zero all cannot be bested */
            if (balc == 0 && inc == 0)
                break;
        }
    }
    
    VERBOSE_3
    (
        fprintf(stderr, "%hu <- %hu -> %hu)   -> bal=%hu, int=%hu\n",
                        cp.l, best, cp.r, bal, in);
        
        draw_set_clip(&cp);
        draw_pause();
    )



    /* Partition (allocates BSP node) */
    cp.pl = cp.pr = best;
    id = select_partition(self, &cp);
    if (id == 0xFFFF)
    {
        fprintf(stderr, "  BSP node allocation failure.\n");
        return 0xFFFF;
    }
    else
    {
        VERBOSE_3
        (
            fprintf(stderr, "  BSP node allocated, id = %hu\n", id);
        )
    }

    /* Draw the scene with the pivot highlighted and its binary partitioning */
    VERBOSE_3
    (
        draw_set_clip(&cp);
        draw_pause();
        draw_set_clip(NULL);
    )

    /* Recurse (and count how much we expand) */
    cparg.l = cp.l;
    cparg.r = cp.pl;
    bsp_insert_left(&g_bsp, id, select_iter(self, &cparg, depth+1));
    i = cparg.r - cp.pl;
    cp.pl += i;
    cp.pr += i;
    cp.r  += i;
    VERBOSE_2
    (
        fprintf(stderr, "Left expansion by %hu.\n", i);
    )
    
    cparg.l = cp.pr+1;
    cparg.r = cp.r;
    bsp_insert_right(&g_bsp, id, select_iter(self, &cparg, depth+1));
    i = cparg.r - cp.r;
    cp.pl += i;
    cp.pr += i;
    cp.r  += i;
    VERBOSE_2
    (
        fprintf(stderr, "Right expansion by %hu.\n", i);
    )

    *pivot = cp;
    return id;
}

void output_tree(void);

bool
select_begin(SELF)
{
    clip_pivot cp;
    
    /* Param check */
    if (!self || !self->verts || !self->faces || !self->planes)
    {
        return false;
    }
    
    /* Reset stats */
    swaps  = 0U;
    rdepth = 0U;
    
    /* Begin iteration */
    cp.l = 0U;
    cp.r = self->n_faces;
    cp.pl = cp.pr = 0U;
    select_iter(self, &cp, 1);
    g_bsp_l = 0;
    g_bsp_r = -1;
    
    /* Print stats */
    printf("Total BSP swaps: %u.\nTotal recursion levels: %u.\n"
           "Total new polys: %u.\n", swaps, rdepth, clips);
    
    output_tree();
    
    return true;
}


void output_tree_node_indent(FILE *f, unsigned short level)
{
    while (level--) fwrite("    ", 1, 4, f);
}

void output_tree_node(FILE *f, unsigned short level, bsp_ind id)
{
    output_tree_node_indent(f, level);
    if (id == 0xFFFF)
    {
        fprintf(f, "NULL\n");
    }
    else if (id >= g_bsp.occ)
    {
        fprintf(f, "<<ERROR> OUT OF BOUNDS>\n");
    }
    else
    {
        bsp_node *pt = g_bsp.d + id;
        
        if (pt->l != 0xFFFF) {
            fprintf(f, "NODE %hd\n", id);
            
            output_tree_node_indent(f, level);
            fprintf(f, "  LEFT {\n");
            output_tree_node(f, level+1, pt->l);
            output_tree_node_indent(f, level);
            fprintf(f, "  }\n");
            
            if (pt->r != 0xFFFF) {
                output_tree_node_indent(f, level);
                fprintf(f, "  RIGHT {\n");
                output_tree_node(f, level+1, pt->r);
                output_tree_node_indent(f, level);
                fprintf(f, "  }\n");
            }
        } else if (pt->r != 0xFFFF) {
            fprintf(f, "NODE %hd\n", id);
            
            output_tree_node_indent(f, level);
            fprintf(f, "  RIGHT {\n");
            output_tree_node(f, level+1, pt->r);
            output_tree_node_indent(f, level);
            fprintf(f, "  }\n");
        } else {
            fprintf(f, "LEAF %hd\n", id);
        }
    }
}

void output_tree(void)
{
    FILE *out = fopen("tree.txt", "w");
    if (!out) return;
    
    output_tree_node(out, 0, 0);
    
    fclose(out);
}
