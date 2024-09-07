#ifndef VERT_H
#define VERT_H

typedef union {
    float m[3];
    struct {
        float x, y, z;
    };
} vert, vertex;

#endif /* VERT_H */
