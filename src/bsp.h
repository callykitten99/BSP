#ifndef PART_H
#define PART_H

#include "pools.h"

#define SELF bsp *const self

typedef unsigned short bsp_ind;

typedef struct {
    bsp_ind s, e;
} pivot;


typedef struct {
    bsp_ind
        p,      /* Parent (node index) */
        l, r;   /* Left and Right Children (node indices) */

    unsigned short
        pl, pr; /* Coplanar Polygons Attached To Node
                 * (polygon vector index range, inclusive) */
} bsp_node;

typedef struct {
    size_t occ, cap;
    bsp_node *d;
} bsp;



static inline void
bsp_init(SELF)
{
    if (!self) return;

    self->d   = NULL;
    self->occ = self->cap = 0U;
}

bool
bsp_alloc(SELF_PARAM(bsp)   out,
          SELF_PARAM(pools) pool);

void bsp_free (SELF);
void bsp_clear(SELF);

bsp_ind bsp_new         (SELF, bsp_ind     pl, bsp_ind    pr);
   void bsp_insert_left (SELF, bsp_ind parent, bsp_ind child);
   void bsp_insert_right(SELF, bsp_ind parent, bsp_ind child);

bsp_node *bsp_node_from_id(SELF, bsp_ind id);

bool
bsp_node_get_faces(SELF_PARAM(bsp)   bsp,
                   SELF_PARAM(pools) pool,
                             pivot  *out,
                             bsp_ind id);

plane *
bsp_node_get_plane(SELF_PARAM(pools) self,
                             bsp_ind id);

#undef SELF
#endif /* PART_H */
