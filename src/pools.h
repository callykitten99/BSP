#ifndef POOLS_H
#define POOLS_H

#include "self.h"
#include "vert.h"
#include "face.h"
#include "plane.h"

#include <stdbool.h>
#include <stddef.h>

#define SELF pools *const self

typedef enum {
    PF_DEL_FACE = 1,
    PF_DEL_PLANE = 2,
} POOL_FLAGS;

typedef struct {
    size_t n_verts, n_faces, /* Number */
           c_verts, c_faces; /* Capacity */
    vert  *verts;
    face  *faces;
    plane *planes;
} pools;



static inline void
pools_init(SELF)
{
    if (!self) return;

    self->n_verts = self->n_faces = 0U;
    self->c_verts = self->c_faces = 0U;
    self->verts   = NULL;
    self->faces   = NULL;
    self->planes  = NULL;
}

bool pools_alloc(SELF, size_t verts, size_t faces);
void pools_free (SELF);
bool pools_check(SELF);

bool pools_vert_declare(SELF, size_t num); /* Declare num verts will be added */
bool pools_face_declare(SELF, size_t num); /* Declare num faces will be added */

unsigned short pools_vert_add  (SELF, vert *v);
unsigned short pools_vert_add_f(SELF, float *v);

unsigned short pools_vert_decl_add   (SELF, vert  *v);
unsigned short pools_vert_decl_add_f (SELF, float *v);
          bool pools_face_decl_insert(SELF, face *f, plane *p,
                                      unsigned short pos); /* p optional */

bool pools_face_add   (SELF, face *f, plane *p); /* p is optional */
bool pools_face_insert(SELF, face *f, plane *p,
                       unsigned short pos); /* p optional*/

bool pools_face_del(SELF, size_t f, POOL_FLAGS);

bool pools_make_planes(SELF);

#undef SELF
#endif /* POOLS_H */
