/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Controls rendering of geometry (via OpenGL).
*******************************************************************************/

#ifndef DRAW_H
#define DRAW_H

#include <stdbool.h>

typedef enum {
    DRAW_MODE_UNSPECIFIED = 0,
    
    /* Polygon table in order, with pivot navigation using `[` `]` (default) */
    DRAW_MODE_LR,
    
    /* Polygon selection debug */
    DRAW_MODE_REL,
    
    /* Polygon clip debug */
    DRAW_MODE_CLIP,
    
    /* Polygon BSP algorithm elimination (/collision detection) debug */
    DRAW_MODE_BSP,
} DRAW_MODE;

void draw_cleanup(void);
bool draw_init(void);
void draw(DRAW_MODE);
void draw_pause(void);
void draw_delay(unsigned long m);
void draw_viewport(unsigned int w, unsigned int h);

#endif /* DRAW_H */
