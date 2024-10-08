/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Simple definition of the vertex (atomic primary data).
*******************************************************************************/

#ifndef VERT_H
#define VERT_H

typedef union {
    float m[3];
    struct {
        float x, y, z;
    };
} vert, vertex;

#endif /* VERT_H */
