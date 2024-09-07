/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Geometry loader.
    Provides access to raw geometric scene from file storage.
*******************************************************************************/

#ifndef DATA_H
#define DATA_H

#include "pools.h"

bool read_data(pools *const restrict out, char const *name);

#endif /* DATA_H */
