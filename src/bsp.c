#include "bsp.h"
#include "verbose.h"

#include <stdio.h>
/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Structures the internal node ordering of the BSP tree.
*******************************************************************************/

#include <stdlib.h>
#include <string.h>

#define SELF bsp *const self



extern pools g_pool;



bool
bsp_alloc(SELF_PARAM(bsp)   out,
          SELF_PARAM(pools) pool)
{
    size_t cap;

    if (!out)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_alloc(bsp *out = NULL).\n");
        )
        return false;
    }
    if (!pool)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_alloc(pools *pool = NULL).\n");
        )
        return false;
    }
    if (!pool->faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_alloc() with no face pool.\n");
        )
        return false;
    }
    if (!pool->planes)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_alloc() with no plane pool.\n");
        )
        return false;
    }

    bsp_free(out);

    cap = pool->n_faces << 1;
    out->d = malloc(cap * sizeof(bsp_node));
    if (!out->d)
    {
        VERBOSE_2
        (
            fprintf(stderr, "Insufficient memory for BSP.\n");
        )
        return false;
    }
    out->cap = cap;
    memset(out->d, 0xFF, cap * sizeof(bsp_node));

    return true;
}



bool
bsp_realloc_(SELF, size_t cap)
{
    size_t old, init;
    bsp_node *reloc;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_realloc(self = NULL)\n");
        )
        return false;
    }
    if (!cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_realloc(cap = 0)\n");
        )
        bsp_free(self);
        return true;
    }
    old = self->cap;
    if (cap == old)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_realloc(cap = old)\n");
        )
        return true;
    }

    init = cap < old ? 0U : (cap - old);

    reloc = realloc(self->d, cap);
    if (!reloc)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR (OOM): bsp_realloc()\n");
        )
        return false;
    }

    memset(reloc + old, 0xFF, init * sizeof(bsp_node));

    self->d   = reloc;
    self->cap = cap;
    if (!init && self->occ > cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_realloc(cap < old)\n");
        )
        self->occ = cap;
    }

    return true;
}



bool
bsp_realloc(SELF, size_t cap)
{
    return self ? bsp_realloc_(self,cap) : false;
}



void bsp_clear(SELF)
{
    if (self && self->d)
        memset(self->d, 0xFF, self->cap * sizeof(bsp_node));
}



void
bsp_free(SELF)
{
    if (!self) return;
    free(self->d);
    bsp_init(self);
}



bool
bsp_expand(SELF)
{
    if (self)
    {
        size_t cap = self->cap;
        
        if (!cap) {
            return bsp_alloc(self, &g_pool);
        }
        else {
            if (cap > 0xFFFF) return NULL;
            cap = (cap<<1) > 0xFFFF ? 0xFFFF : (cap<<1);
            return bsp_realloc_(self, cap);
        }
    }
    return NULL;
}



bsp_ind
bsp_new(SELF, bsp_ind pl, bsp_ind pr)
{
    bsp_ind id;
    bsp_node   *p;
    
    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_new(self = NULL)\n");
        )
        return 0xFFFF;
    }
    if (self->occ >= self->cap && !bsp_expand(self))
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: bsp_new() expansion failure.\n");
        )
        return 0xFFFF;
    }
    
    id = (size_t)(self->occ++);
    p = self->d + id;
    p->pl = pl;
    p->pr = pr;
    p->p  = 0xFFFF;
    p->l  = 0xFFFF;
    p->r  = 0xFFFF;
    
    return id;
}



void
bsp_insert_left(SELF, bsp_ind parent, bsp_ind child)
{
    size_t e;
    bsp_node *par, *cld;
    
    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_insert_left(self = NULL)\n");
        )
        return;
    }
    
    e = self->occ;
    if (parent >= e)
    {
        VERBOSE_2
        (
            fprintf(stderr,
                    "WARNING: bsp_insert_left(parent=%hu out of bounds)\n",
                    parent);
        )
        return;
    }
    if (child >= e && child != 0xFFFF)
    {
        VERBOSE_2
        (
            fprintf(stderr,
                    "WARNING: bsp_insert_left(child=%hu out of bounds)\n",
                    parent);
        )
        return;
    }
    
    par = self->d + parent;
    cld = self->d + child;
    
    par->l = child;
    if (child != 0xFFFF)
        cld->p = parent;
}



void
bsp_insert_right(SELF, bsp_ind parent, bsp_ind child)
{
    size_t e;
    bsp_node *par, *cld;
    
    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: bsp_insert_right(self = NULL)\n");
        )
        return;
    }
    
    e = self->occ;
    if (parent >= e)
    {
        VERBOSE_2
        (
            fprintf(stderr,
                    "WARNING: bsp_insert_right(parent=%hu out of bounds)\n",
                    parent);
        )
        return;
    }
    if (child >= e && child != 0xFFFF)
    {
        VERBOSE_2
        (
            fprintf(stderr,
                    "WARNING: bsp_insert_right(child=%hu out of bounds)\n",
                    parent);
        )
        return;
    }
    
    par = self->d + parent;
    cld = self->d + child;
    
    par->r = child;
    if (child != 0xFFFF)
        cld->p = parent;
}



bsp_node *
bsp_node_from_id(SELF, bsp_ind id)
{
    return (self && id < self->occ) ? self->d + id : NULL;
}



bool
bsp_node_get_faces(SELF_PARAM(bsp)   self,
               SELF_PARAM(pools) pool,
               pivot  *out,
               bsp_ind id)
{
    size_t mf;
    bsp_node *p;
    
    if (!out  ||
        !self  || !self->d || id >= self->occ ||
        !pool || !pool->faces)
    {
        return false;
    }
    
    mf = pool->n_faces;
    
    p = self->d + id;
    if (p->pl < mf && p->pr < mf)
    {
        out->s = p->pl;
        out->e = p->pr;
        return true;
    }
    return false;
}



plane *
bsp_node_get_plane(SELF_PARAM(pools) self, bsp_ind id)
{
    if (!self || !self->planes || (size_t)id >= self->n_faces)
    {
        return NULL;
    }

    return self->planes + id;
}
