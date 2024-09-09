/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Facilitates interaction with the GUI window.
*******************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

void window_cleanup(void);

bool window_init_default(void);
bool window_init(unsigned int w, unsigned int h);
bool window_loop(void);

float window_get_aspect_ratio(void);

#endif /* WINDOW_H */
