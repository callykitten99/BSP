/*******************************************************************************
    Binary Spatial Partitioning Algorithm
        Author: Callum David Ames               All Rights Reserved
        Date Initiated: July 2024
    
    Defines program entry point and exit clean-up routines.
*******************************************************************************/

#include "bsp.h"
#include "data.h"
#include "draw.h"
#include "select.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>


void usage(void);
void cleanup(void);


pools g_pool;
bsp   g_bsp;


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    atexit(cleanup);
    pools_init(&g_pool);
    bsp_init(&g_bsp);

    if (!read_data(&g_pool, argv[1]))
    {
        fprintf(stderr, "Exiting due to data read error.\n");
        return EXIT_FAILURE;
    }

    if (!pools_check(&g_pool))
    {
        fprintf(stderr, "Exiting due to pool check error.\n");
        return EXIT_FAILURE;
    }

    if (!pools_make_planes(&g_pool))
    {
        fprintf(stderr, "Exiting due to plane error.\n");
        return EXIT_FAILURE;
    }

    window_init_default();
    draw_init();

    while (window_loop());
}

void cleanup(void)
{
    draw_cleanup();
    window_cleanup();
    bsp_free(&g_bsp);
    pools_free(&g_pool);
}

void usage(void)
{
    fprintf(stderr, "Usage:\n  bsp obj_name\n");
}

