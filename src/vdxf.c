#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "dxf.h"

int main(int argc, char **argv) {
    int i;

    /* Do we have 2 parameters? */
    if(argc < 2) {
        /* No, print usage and exit */
        fprintf(stderr, "Usage: %s <dxf_filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for(i = 1;i < argc; i++) {
        dxf_handle_t dxf; /* API handle */
        dxf_error_t err; /* error code */
        /* Load the DXF file */
        if((err = dxf_load(&dxf, argv[i])) != dxfErrorOk) {
            /* Failed, print error and exit */
            (void)dxf_print_error(err, stderr);
        } else {
            dxf_print(dxf, stdout);
            dxf_unload(dxf);
        }
    }
    return 0;
}

