/*#include <gtk/gtk.h>*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "dxf.h"
#include "sdict.h"

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
        dxf_error_code_t err; /* error code */
        printf("loading %s...\n", argv[i]);
        /* Load the DXF file */
        if((err = dxf_load(&dxf, argv[i])) != dxfErrorOk) {
            /* Failed, print error and exit */
            (void)dxf_print_error(err, stderr);
        } else {
            dxf_unload(dxf);
        }
    }
    return 0;
}

int xmain(int argc, char **argv) {
    dxf_handle_t dxf; /* API handle */
    dxf_error_code_t err; /* error code */

    /* Do we have 2 parameters? */
    if(argc < 2) {
        /* No, print usage and exit */
        fprintf(stderr, "Usage: %s <dxf_filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Load the DXF file */
    if((err = dxf_load(&dxf, argv[1])) != dxfErrorOk) {
        /* Failed, print error and exit */
        (void)dxf_print_error(err, stderr);
        exit(EXIT_FAILURE);
    }
    if(argc == 3) {
        if(dxf_has_var(dxf, argv[2]) == dxfErrorOk) {
            printf("Has %s\n", argv[2]);
        } else {
            printf("Does NOT have %s\n", argv[2]);
        }
    }
    dxf_unload(dxf);
    exit(EXIT_SUCCESS);
}
/*
int main(int argc, char *argv[]) {
    GtkWidget *window;
    
    gtk_init (&argc, &argv);
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_show  (window);
    
    gtk_main ();
    return 0;
}
*/    

