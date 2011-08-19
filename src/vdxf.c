/*#include <gtk/gtk.h>*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "dxf.h"
#include "sdict.h"

int main(int argc, char **argv) {
    dxf_t dxf; /* DXF state structure */

    /* Do we have 2 parameters? */
    if(argc < 2) {
        /* No, print usage and exit */
        fprintf(stderr, "Usage: %s <dxf_filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Reset structure */
    memset(&dxf, 0, sizeof(dxf_t));

    /* Load the DXF file */
    if(dxf_load(&dxf, argv[1]) != 1) {
        /* Failed, print error and exit */
        dxf_print_last_error(&dxf);
        exit(EXIT_FAILURE);
    }
    if(argc == 3) {
        if(dxf_has_var(&dxf, argv[2])) {
            printf("Has %s\n", argv[2]);
        } else {
            printf("Does NOT have %s\n", argv[2]);
        }
    }
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

