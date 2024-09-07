/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Geometry loader.
    Provides access to raw geometric scene from file storage.
*******************************************************************************/

#include "data.h"

#include <stdio.h>


typedef enum {
    FST_VERT = 0,
    FST_FACE,
} FILE_SRC_TYPE;


#if 0
static size_t
fsize(FILE *f)
{
    fpos_t pos;
    size_t r;

    if (!f) return 0U;

    fgetpos(f, &pos);
    fseek(f, 0, SEEK_END);
    r = ftell(f);
    fsetpos(f, &pos);
    return r;
}
#endif

static bool
fopen_ex(char const *name,
         FILE **f_out,
         size_t *sz_out,
         FILE_SRC_TYPE type)
{
    char buf[256];
    FILE *f;
    char *dot = NULL, *pc;
    size_t sz;

    if (!name || !f_out || !sz_out) return false;

    /* Replace extension on name */
    for (pc = buf; *name && pc < (buf+255); ++pc, ++name)
    {
        *pc = *name;
        if (*pc == '.') dot = pc+1;
    }
    *pc = '\0';
    if (!dot) {
        *(pc++) = '.';
        dot = pc;
    }
    if (dot < (buf+253)) {
        switch (type) {
        case FST_VERT:
            dot[0] = 'V';
            dot[1] = 'T';
            dot[2] = 'X';
            dot[3] = '\0';
            break;

        case FST_FACE:
            dot[0] = 'I';
            dot[1] = 'D';
            dot[2] = 'X';
            dot[3] = '\0';
            break;

        default: return false;
        }
    }
    fprintf(stderr, "opening \"%s\".\n", buf);

    /* Open file */
    f = fopen(buf, "rb");
    if (!f) return false;

    /* Determine file size */
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    rewind(f);

    fprintf(stderr, "File size: %zu.\n", sz);

    *f_out = f;
    *sz_out = sz;
    return true;
}

bool
read_data(pools *const restrict out,
          char const *name)
{
    FILE *fv = NULL, *ff = NULL;
    size_t fvsz, ffsz, elems;

    if (!out) return false;

    if (!fopen_ex(name, &fv, &fvsz, FST_VERT))
    {
        fprintf(stderr, "Failed to open vertex file \"%s\".\n", name);
        return false;
    }

    if (!fopen_ex(name, &ff, &ffsz, FST_FACE))
    {
        fprintf(stderr, "Failed to open index file \"%s\".\n", name);
        goto L_Error;
    }

    if (!pools_alloc(out, fvsz / sizeof(vert), ffsz / sizeof(face)))
    {
        fprintf(stderr, "Failed to allocate pools.\n");
        goto L_Error;
    }

    if (out->n_verts !=
            (elems = fread(
                out->verts,
                sizeof(vert),
                out->n_verts,
                fv)    )    )
    {
        fprintf(stderr, "Vertex read failure.\n");
        goto L_Error;
    }
    fclose(fv); fv = NULL;

    if (out->n_faces != 
            (elems = fread(
                out->faces,
                sizeof(face),
                out->n_faces,
                ff)    )    )
    {
        fprintf(stderr, "Index read failure.\n");
        goto L_Error;
    }
    fclose(ff); ff = NULL;

    return true;

L_Error:
    if (fv) fclose(fv);
    if (ff) fclose(ff);
    return false;
}

