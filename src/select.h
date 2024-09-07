/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Defines subroutines related to:
    - pivot selection
    - in-place rebalancing of triangles (and corresponding
       Hessian-Normal-Form plane aggregate data) relative to pivot
    - coordinated construction of BSP nodes
*******************************************************************************/

#ifndef SELECT_H
#define SELECT_H

#include "pools.h"

bool select_begin(SELF_PARAM(pools) pool);

#endif /* SELECT_H */
