#include "pools.h"
#include "math.h"
#include "verbose.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SELF pools *const self


void
pools_free(SELF)
{
    free(self->verts);
    free(self->faces);
    free(self->planes);
    pools_init(self);
}

bool
pools_alloc(SELF, size_t verts, size_t faces)
{
    if (!self) return false;

    pools_free(self);

    self->verts  = calloc(verts, sizeof(vert));
    self->faces  = calloc(faces, sizeof(face));
    self->planes = calloc(faces, sizeof(plane));

    if (!self->verts || !self->faces || !self->planes)
    {
        pools_free(self);
        return false;
    }

    self->n_verts = self->c_verts = verts;
    self->n_faces = self->c_faces = faces;
    return true;
}

bool
pools_realloc_faces_(SELF, size_t cap)
{
    size_t init = 0U, old;
    face  *n_f;
    plane *n_p;

    if (!cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_faces(cap = 0)\n");
        )
        free(self->faces);
        free(self->planes);
        self->faces   = NULL;
        self->planes  = NULL;
        self->n_faces = 0U;
        self->c_faces = 0U;
        return true;
    }
    old = self->c_faces;
    if (cap == old)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_faces(cap = old)\n");
        )
        return true;
    }
    if (cap  > old)
        init = cap - old;

    n_f = realloc(self->faces, cap*sizeof(face));
    if (!n_f)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR (OOM): pools_realloc_faces(%zu faces)\n",
                            cap);
        )
        return false;
    }
    memset(n_f + old, 0, init*sizeof(face));
    self->faces = n_f;
    
    n_p = realloc(self->planes, cap*sizeof(plane));
    if (!n_p)
    {
        self->c_faces = old;
        
        VERBOSE_2
        (
            fprintf(stderr, "ERROR (OOM): pools_realloc_faces(%zu planes)\n",
                            cap);
        )
        return false;
    }
    memset(n_p + old, 0, init*sizeof(plane));
    self->planes  = n_p;
    self->c_faces = cap;

    if (!init && self->n_faces > cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_faces(cap < old)\n");
        )
        self->n_faces = cap;
    }

    VERBOSE_2
    (
        fprintf(stderr, "OK: pools_realloc_faces(%zu faces).\n", cap);
    )
    return true;
}

bool
pools_realloc_faces(SELF, size_t cap)
{
    return self ? pools_realloc_faces_(self, cap) : false;
}

bool
pools_realloc_verts_(SELF, size_t cap)
{
    size_t init = 0U, old;
    vert *nmem;

    if (!cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_verts(cap = 0)\n");
        )
        free(self->verts);
        self->verts = NULL;
        self->n_verts = 0U;
        self->c_verts = 0U;
        return true;
    }
    old = self->c_verts;
    if (cap == old)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_verts(cap = old cap)\n");
        )
        return true;
    }
    if (cap  > old)
        init = cap - old;

    nmem = realloc(self->verts, cap*sizeof(vert));
    if (!nmem)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR (OOM): pools_realloc_verts(cap = %zu).\n",
                    cap);
        )
        return false;
    }

    memset(nmem + old, 0, init*sizeof(vert));
    self->verts = nmem;
    self->c_verts = cap;

    if (!init && self->n_verts > cap)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_verts(cap < old).\n");
        )
        self->n_verts = cap;
    }

    VERBOSE_2
    (
        fprintf(stderr, "OK: pools_realloc_verts(%zu verts).\n", cap);
    )
    return true;
}

bool
pools_realloc_verts(SELF, size_t cap)
{
    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_verts(self = NULL).\n");
        )
        
        return false;
    }
    if (cap > 0xFFFF)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_realloc_verts(cap = %zu > limit).\n", cap);
        )
        
        return false;
    }

    return pools_realloc_verts_(self, cap);
}

bool
pools_expand_verts(SELF)
{
    size_t cap;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "Call to expand vertex pool without SELF.\n");
        )
        return false;
    }

    cap = self->c_verts;
    if (cap >= 0xFFFF)
    {
        VERBOSE_2
        (
            fprintf(stderr, "Call to expand vertex pool exceeds limit.\n");
        )
        return false;
    }

    cap = (cap<<1) > 0xFFFF ? 0xFFFF : (cap<<1); 
    return pools_realloc_verts_(self, cap);
}

bool
pools_expand_faces(SELF)
{
    return self ? pools_realloc_faces_(self, self->c_faces<<1) : false;
}

unsigned short
pools_vert_add(SELF, vert *v)
{
    size_t nv;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_add(self = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!v)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_add(vertex = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!self->verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_add() called with no existing "
                            "vertex pool.\n");
        )
        return 0xFFFF;
    }
    if (self->n_verts >= self->c_verts &&
        !pools_expand_verts(self))
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_add() pool expansion failure.\n");
        )
        return 0xFFFF;
    }

    nv = self->n_verts++;
    self->verts[nv] = *v;

    return nv;
}

unsigned short
pools_vert_add_f(SELF, float *v)
{
    size_t nv;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_add_f(self = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!v)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_add_f(vertex = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!self->verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_add_f() called with no existing "
                            "vertex pool.\n");
        )
        return 0xFFFF;
    }
    if (self->n_verts >= self->c_verts &&
        !pools_expand_verts(self))
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_add_f() pool expansion failure.\n");
        )
        return 0xFFFF;
    }

    nv = self->n_verts++;
    memcpy(self->verts[nv].m, v, sizeof(float)*3);
    
    VERBOSE_3
    (
        fprintf(stderr, "Vertex added.\n");
    )
    return nv;
}

bool
pools_vert_declare(SELF, size_t num)
{
    if (!self)
    {
        VERBOSE_2(fprintf(stderr, "WARNING: pools_vert_declare(self = NULL)\n");)
        return false;
    }
    if (self->n_verts + num >= self->c_verts &&
        !pools_expand_verts(self))
    {
        return false;
    }
    return true;
}

bool
pools_face_declare(SELF, size_t num)
{
    if (!self)
    {
        VERBOSE_2(fprintf(stderr, "WARNING: pools_face_declare(self = NULL)\n");)
        return false;
    }
    if (self->n_faces + num >= self->c_faces &&
        !pools_expand_faces(self))
    {
        return false;
    }
    return true;
}

unsigned short
pools_vert_decl_add(SELF, vert *v)
{
    size_t nv;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_decl_add(self = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!v)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_decl_add(vertex = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!self->verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_decl_add() called with no existing "
                            "vertex pool.\n");
        )
        return 0xFFFF;
    }
    if (self->n_verts >= self->c_verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_decl_add() would expand.\n");
        )
        return 0xFFFF;
    }

    nv = self->n_verts++;
    self->verts[nv] = *v;
    
    VERBOSE_3
    (
        fprintf(stderr, "Vertex added.\n");
    )
    return nv;
}

unsigned short
pools_vert_decl_add_f(SELF, float *v)
{
    size_t nv;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_decl_add_f(self = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!v)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_vert_decl_add_f(vertex = NULL)\n");
        )
        return 0xFFFF;
    }
    if (!self->verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_decl_add_f() called with no existing "
                            "vertex pool.\n");
        )
        return 0xFFFF;
    }
    if (self->n_verts >= self->c_verts)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_vert_decl_add_f() would expand.\n");
        )
        return 0xFFFF;
    }

    nv = self->n_verts++;
    memcpy(self->verts[nv].m, v, sizeof(float)*3);
    
    VERBOSE_3
    (
        fprintf(stderr, "Vertex added.\n");
    )
    return nv;
}

/* p optional */
bool
pools_face_decl_insert(SELF, face *f, plane *p, unsigned short pos)
{
    size_t nf, i;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_decl_insert(self = NULL)\n");
        )
        return false;
    }
    if (!f)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_decl_insert(face = NULL)\n");
        )
        return false;
    }
    if ((size_t)pos > self->n_faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_decl_insert(face=%hu > nfaces=%zu)\n",
                            pos, self->n_faces);
        )
        return false;
    }
    if (!self->faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_decl_insert() called with no existing "
                            "face pool.\n");
        )
        return false;
    }
    if (!self->planes)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_decl_insert() called with no existing "
                            "plane pool.\n");
        )
        return false;
    }
    if (self->n_faces >= self->c_faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_decl_insert() would expand.\n");
        )
        return false;
    }

    nf = self->n_faces++;

    /* Move elements to the right */
    for (i = nf; i > pos; --i)
        self->faces[i] = self->faces[i-1];
    for (i = nf; i > pos; --i)
        self->planes[i] = self->planes[i-1];

    /* Emplace */
    self->faces[pos] = *f;
    if (!p) {
        plane_from_face(self->planes + pos, self->verts, f);
    } else {
        self->planes[pos] = *p;
    }

    VERBOSE_3
    (
        fprintf(stderr, "Face inserted.\n");
    )
    return true;
}

/* p is optional */
bool
pools_face_add(SELF, face *f, plane *p)
{
    size_t nf;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_add(self = NULL)\n");
        )
        return false;
    }
    if (!f)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_add(face = NULL)\n");
        )
        return false;
    }
    if (!self->faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_add() called with no existing "
                            "face pool.\n");
        )
        return false;
    }
    if (!self->planes)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_add() called with no existing "
                            "plane pool.\n");
        )
        return false;
    }
    if (self->n_faces >= self->c_faces &&
        !pools_expand_faces(self))
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_add() pool expansion failure.\n");
        )
        return false;
    }

    nf = self->n_faces++;
    self->faces[nf] = *f;

    if (!p) {
        plane_from_face(self->planes+nf, self->verts, f);
    } else {
        self->planes[nf] = *p;
    }
    
    VERBOSE_3
    (
        fprintf(stderr, "Face added.\n");
    )
    return true;
}

/* p is optional */
bool
pools_face_insert(SELF, face *f, plane *p, unsigned short pos)
{
    size_t nf, i;

    if (!self)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_insert(self = NULL)\n");
        )
        return false;
    }
    if (!f)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_insert(face = NULL)\n");
        )
        return false;
    }
    if ((size_t)pos > self->n_faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "WARNING: pools_face_insert(face=%hu > nfaces=%zu)\n",
                            pos, self->n_faces);
        )
        return false;
    }
    if (!self->faces)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_insert() called with no existing "
                            "face pool.\n");
        )
        return false;
    }
    if (!self->planes)
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_insert() called with no existing "
                            "plane pool.\n");
        )
        return false;
    }
    if (self->n_faces >= self->c_faces &&
        !pools_expand_faces(self))
    {
        VERBOSE_2
        (
            fprintf(stderr, "ERROR: pools_face_insert() pool expansion failure.\n");
        )
        return false;
    }

    nf = self->n_faces++;

    /* Move elements to the right */
    for (i = nf; i > pos; --i)
        self->faces[i] = self->faces[i-1];
    for (i = nf; i > pos; --i)
        self->planes[i] = self->planes[i-1];

    /* Emplace */
    self->faces[pos] = *f;
    if (!p) {
        plane_from_face(self->planes + pos, self->verts, f);
    } else {
        self->planes[pos] = *p;
    }

    VERBOSE_3
    (
        fprintf(stderr, "Face inserted.\n");
    )
    return true;
}

bool
pools_check(SELF)
{
    size_t i, j, nv;
    face *faces;

    if (!self ||
        !self->verts   || !self->faces || !self->planes ||
        !self->n_verts || !self->n_faces)
    {
        return false;
    }

    faces = self->faces;
    nv    = self->n_verts;

    fprintf(stderr, "Num verts: %zu.\nNum faces: %zu.\n", nv, self->n_faces);

    for (i = 0, j = self->n_faces;      i < j;      ++i)
    {
        if (faces[i].i[0] >= nv ||
            faces[i].i[1] >= nv ||
            faces[i].i[2] >= nv)
        {
            fprintf(stderr,
                    "Face %zu contains an index that exceeds the vertex pool "
                    "and will be deleted.\n", i);

            pools_face_del(self, i, PF_DEL_FACE);
            --i; --j;
        }
    }

    return true;
}

bool
pools_face_del(SELF, size_t f, POOL_FLAGS pf)
{
    size_t i, j;

    if (!self ||
        !self->faces || !self->planes ||
        f >= self->n_faces)
    {
        return false;
    }

    j = self->n_faces;
    if (pf & PF_DEL_FACE)
    {
        face *const ptr = self->faces;
        for (i = f+1; i < j; ++i)
        {
            ptr[i-1] = ptr[i];
        }
    }

    if (pf & PF_DEL_PLANE)
    {
        plane *const ptr = self->planes;
        for (i = f+1; i < j; ++i)
        {
            ptr[i-1] = ptr[i];
        }
    }

    -- self->n_faces;
    return true;
}

bool
pools_make_planes(SELF)
{
    size_t i, j;

    if (!self ||
        !self->verts   || !self->faces || !self->planes ||
        !self->n_verts || !self->n_faces)
    {
        return false;
    }

    for (i = 0, j = self->n_faces;      i < j;      ++i)
    {
        if (!plane_from_face(self->planes+i,
                             self->verts,
                             self->faces+i))
        {
            fprintf(stderr,
                    "Face %zu does not form a plane and will be deleted.\n",
                    i);

            pools_face_del(self, i, PF_DEL_FACE);
            --i; --j;
        }
#if 0
        else
        {
            plane *ptr = self->planes+i;

            printf("{d = % 8.2f, % 1.4f, % 1.4f, % 1.4f}\n",
                   ptr->d, ptr->x, ptr->y, ptr->z);
        }
#endif
    }

    return true;
}


