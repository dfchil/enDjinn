/**
 * Example program demonstrating enDJinn usage
 * 
 * This creates a simple rotating square demo
 */

#include "endgine.h"
#include <kos.h>
#include <math.h>

/* Demo state */
static struct {
    float rotation;
    pvr_ptr_t texture;
} demo_state;

/* Initialize the demo */
static int demo_init(void) {
    demo_state.rotation = 0.0f;
    demo_state.texture = NULL;
    
    printf("enDJinn Demo Initialized\n");
    printf("Press START to exit\n");
    
    return 0;
}

/* Update the demo state */
static void demo_update(float delta_time) {
    /* Rotate at 45 degrees per second */
    demo_state.rotation += 45.0f * delta_time;
    if (demo_state.rotation >= 360.0f) {
        demo_state.rotation -= 360.0f;
    }
}

/* Render the demo */
static void demo_render(void) {
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_vertex_t vert;
    float x, y;
    float size = 100.0f;
    float cx = 320.0f; /* Center X */
    float cy = 240.0f; /* Center Y */
    float rad = demo_state.rotation * F_PI / 180.0f;
    
    /* Set up the context for a colored polygon */
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));
    
    /* Draw a rotating square */
    /* Vertex 1 */
    x = cx + (cosf(rad) * size - sinf(rad) * size);
    y = cy + (sinf(rad) * size + cosf(rad) * size);
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y;
    vert.z = 1.0f;
    vert.argb = 0xFFFF0000; /* Red */
    pvr_prim(&vert, sizeof(vert));
    
    /* Vertex 2 */
    x = cx + (cosf(rad) * -size - sinf(rad) * size);
    y = cy + (sinf(rad) * -size + cosf(rad) * size);
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y;
    vert.z = 1.0f;
    vert.argb = 0xFF00FF00; /* Green */
    pvr_prim(&vert, sizeof(vert));
    
    /* Vertex 3 */
    x = cx + (cosf(rad) * -size - sinf(rad) * -size);
    y = cy + (sinf(rad) * -size + cosf(rad) * -size);
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;
    vert.y = y;
    vert.z = 1.0f;
    vert.argb = 0xFF0000FF; /* Blue */
    pvr_prim(&vert, sizeof(vert));
    
    /* Vertex 4 (end of strip) */
    x = cx + (cosf(rad) * size - sinf(rad) * -size);
    y = cy + (sinf(rad) * size + cosf(rad) * -size);
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x;
    vert.y = y;
    vert.z = 1.0f;
    vert.argb = 0xFFFFFF00; /* Yellow */
    pvr_prim(&vert, sizeof(vert));
}

/* Cleanup the demo */
static void demo_cleanup(void) {
    printf("enDJinn Demo Cleanup\n");
}

/* Main entry point */
int main(int argc, char **argv) {
    /* Configure the engine */
    endgine_config_t config = {
        .init = demo_init,
        .update = demo_update,
        .render = demo_render,
        .cleanup = demo_cleanup,
        .target_fps = 60
    };
    
    /* Run the game loop */
    int result = endgine_run(&config);
    
    return result;
}
