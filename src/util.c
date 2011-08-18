#include <stdio.h>
#include <ctype.h>
#include <assert.h>

void util_trim(char *s) {
    int i; /* Iterator */
    int idx = 0; /* Current string index */
    int state = 0; /* Current processing state */

    /* String valid? */
    assert(s != NULL);
    /* Loop over all characters, moving in-place as needed */
    for(i = 0; s[i] != '\0'; i++) {
        /* Which state are we in? */
        if(state == 0) {
            /* Leading state, do we have whitespace? */
            if(isspace(s[i]) != 0) {
                /* Whitespace, continue */
                continue;
            }
            /* Non-whitespace found, save it and change state */
            s[idx++] = s[i];
            state = 1;
        } else {
            /* Non-leading state, do we have whitespace? */
            if(isspace(s[i]) != 0) {
                /* Whitespace, we're done */
                s[i] = '\0';
                break;
            }
            /* Non-whitespace, save it and continue */
            s[idx++] = s[i];
        }
    }
    /* Terminate the string */
    s[idx] = '\0';
}
