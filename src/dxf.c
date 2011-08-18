/*
group code/value

sections
    records
        group code
        data item

section starts with group code 0, SECTION
    group code 2, name of section
section ends with group code 0, ENDSEC
*/

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include "dxf.h"
#include "dxf_types.h"
#include "util.h"

extern int dxf_group_type_map[];

/* Max line length according to DXF manual, not including NL */
#define DXF_MAX_LINE_LENGTH 2049
#define DXF_RESET_ERROR(a) a->error.code = dxfErrorOk;a->error.msg[0] = '\0';
#define DXF_RESET(a) DXF_RESET_ERROR(a);a->line=0;a->column=0;\
    a->filename[0]='\0';
#define SET_ERROR(dxf, c) dxf->error.code = c;(void)snprintf(dxf->error.msg, \
    sizeof(dxf->error.msg), "%s", DXF_ERROR_S[dxf->error.code]);
#define SET_ERRNO_ERROR(dxf, c) SET_ERROR(dxf, c); \
        (void)snprintf(dxf->error.msg, sizeof(dxf->error.msg), "%s : %s", \
            dxf->error.msg, strerror(errno));

/* Error strings */
static char *DXF_ERROR_S[] = {
    "OK",
    "Bad file descriptor",
    "Invalid Format",
    "End of file",
    "Read error (fgets)",
    "Line exceeds max length",
    "Non-ASCII character",
    "Digit expected",
    "Invalid file",
    "Open failed",
    "Close failed",
    "Snprintf failed"
};

typedef struct _range_to_type_t {
    int start;
    int end;
    char type_name[32];
} range_to_type_t;

/*
If you modify this map, generate and use udpated source code by:
    - calling dxf_print_type_map()
    - paste output over value in dxf_types.c
*/
static range_to_type_t g_range_to_types [] = {
    { 0, 9, "dxfString2049" },
    { 10, 39, "dxfDouble" },
    { 40, 59, "dxfDouble" },
    { 60, 79, "dxfInt16" },
    { 90, 99, "dxfInt32" },
    { 100, 100, "dxfString255" },
    { 102, 102, "dxfString255" },
    { 105, 105, "dxfString255" },
    { 110, 119, "dxfDouble" },
    { 120, 129, "dxfDouble" },
    { 130, 139, "dxfDouble" },
    { 140, 149, "dxfDouble" },
    { 160, 169, "dxfInt64" },
    { 170, 179, "dxfInt16" },
    { 210, 239, "dxfDouble" },
    { 270, 279, "dxfInt16" },
    { 280, 289, "dxfInt16" },
    { 290, 299, "dxfBoolean" },
    { 300, 309, "dxfString2049" },
    { 310, 319, "dxfString2049" },
    { 320, 329, "dxfString255" },
    { 330, 369, "dxfString255" },
    { 370, 379, "dxfInt16" },
    { 380, 389, "dxfInt16" },
    { 390, 399, "dxfString255" },
    { 400, 409, "dxfInt16" },
    { 410, 419, "dxfInt16" },
    { 420, 429, "dxfInt32" },
    { 430, 439, "dxfString2049" },
    { 440, 449, "dxfInt32" },
    { 450, 459, "dxfLong" },
    { 460, 469, "dxfDouble" },
    { 470, 479, "dxfString2049" },
    { 480, 481, "dxfString255" },
    { 999, 999, "dxfString2049" },
    { 1000, 1009, "dxfString2049" },
    { 1010, 1059, "dxfDouble" },
    { 1060, 1070, "dxfInt16" },
    { 1071, 1071, "dxfInt32" }
};

static int dxf_type_name_to_enum(const char *name) {
    /*dxfString2049, dxfDouble, dxfInt16, dxfInt32,
        dxfString255, dxfInt64, dxfBoolean, dxfLong */
    if(strcmp(name, "dxfString2049") == 0) {
        return 0;
    } else if(strcmp(name, "dxfDouble") == 0) {
        return 1;
    } else if(strcmp(name, "dxfInt16") == 0) {
        return 2;
    } else if(strcmp(name, "dxfInt32") == 0) {
        return 3;
    } else if(strcmp(name, "dxfString255") == 0) {
        return 4;
    } else if(strcmp(name, "dxfInt64") == 0) {
        return 5;
    } else if(strcmp(name, "dxfBoolean") == 0) {
        return 6;
    } else if(strcmp(name, "dxfLong") == 0) {
        return 7;
    } else {
        fprintf(stderr, "Invalid group type name: %s\n", name);
        exit(EXIT_FAILURE);
    }
}

void dxf_print_type_map() {
    int cnt = (int)(sizeof(g_range_to_types)/sizeof(range_to_type_t));
    int i;
    int min = INT_MAX;
    int max = INT_MIN;
    int total = 0;
    int *array;
    int array_len;

    /* get min/max array bounds */
    for(i = 0; i < cnt; i++) {
        range_to_type_t t = g_range_to_types[i];
        if(t.start < min) {
            min = t.start;
        }
        if(t.end > max) {
            max = t.end;
        }
    }

    /* Allocate array big enough for entire range of group codes  */
    array_len = max + 1;
    array = (int*)malloc(array_len * sizeof(int));
    assert(array != NULL);
    memset(array, -1, array_len * sizeof(int));

    /* Populate the (sparse) array */
    for(i = 0; i < cnt; i++) {
        int j;
        range_to_type_t t = g_range_to_types[i];
        for(j = t.start; j <= t.end; j++) {
            array[j] = dxf_type_name_to_enum(t.type_name);
        }
    }

    /* Print the array initialization */
    printf("/* Array of integers where index is group code\n");
    printf("and value is enumeration indicating data type associated\n");
    printf("with that group code. */\n");
    printf("int dxf_group_type_map [] = {\n");
    for(i = 0; i < array_len; i++) {
        printf("    %i", array[i]);
        if((total + 1) < (max + 1)) {
            printf(",");
        }
        printf("\n");
        total++;
    }
    printf("};\n");
    free(array);
}

/**
Parses a single data item from a DXF stream.

@param  dxf DXF state structure.
@param  fp  Open stream.
@param  value   Character buffer at least (DXF_MAX_LINE_LENGTH+1) bytes.
    On success, contains the parsed value.
@param  id  If 1, item expected to be an id, therefore all digits.
    If 0, expected to be regular value, therefore any ASCII.
@returns 1 on success, 0 on error.
*/
static int dxf_parse_item(dxf_t *dxf, FILE *fp, char *value, int id) {
    char line[DXF_MAX_LINE_LENGTH * 2]; /* Line read buffer */
    int i; /* Iterator */
    int len; /* Temp length */
    int line_len = 0; /* Line length */

    /* Read a line from the stream */
    if(fgets(line, (int)sizeof(line), fp) == NULL) {
        /* Error */
        if(feof(fp) != 0) {
            SET_ERROR(dxf, dxfErrorEOF);
        } else {
            SET_ERRNO_ERROR(dxf, dxfErrorFgets);
        }
        return 0;
    }

    /* Keep track of current line number */
    dxf->line++;

    /* Trim leading/trailing whitespace */
    util_trim(line);

    /* Loop over each character */
    len = (int)strlen(line);
    for(i = 0; i < len; i++) {
        /* Keep track of the current column */
        dxf->column = i;

        /* No value length may exceed max and all chars must be ASCII */
        if(i == DXF_MAX_LINE_LENGTH) {
            SET_ERROR(dxf, dxfErrorLineTooLong);
            return 0;
        } else if(isascii((int)line[i]) != 1) {
            SET_ERROR(dxf, dxfErrorNonASCII);
            return 0;
        }

        /* If id, must be all digits */
        if((id == 1) && (isdigit(line[i]) != 1)) {
            dxf->error.code = dxfErrorDigitExpected;
            return 0;
        } 

        /* Save the character and continue */
        value[line_len++] = line[i];
    }

    /* Terminate the value */
    value[line_len] = '\0';
    return 1;
}

/**
Parses a single record from a DXF stream.

@param  dxf DXF state structure.
@param  fp  Open stream.
@param  group_code  On success, contains the parsed group_code.
@param  value  On success, contains the parsed value.
@returns 1 on success, 0 on error.
*/
static int dxf_parse_record(dxf_t *dxf, FILE *fp, int *group_code,
    char *value) {
    char buf[DXF_MAX_LINE_LENGTH + 1]; /* Temp buffer */

    /* Parse the group_code */
    buf[0] = '\0';
    if(dxf_parse_item(dxf, fp, buf, 1) != 1) {
        /* Error */
        return 0;
    }
    *group_code = atoi(buf);

    /* Parse the value */
    if(dxf_parse_item(dxf, fp, value, 0) != 1) {
        /* Error */
        return 0;
    }

    return 1;
}

/**
Attempts to load a DXF file by filename.

@param  dxf DXF state structure.
@param  filename    Filename.
@returns 1 on success, 0 on error.
*/
int dxf_load(dxf_t *dxf, const char *filename) {
    struct stat statbuf; /* Struct for stat() call */
    int fd; /* File descriptor */

    /* Is the structure valid? */
    assert(dxf != NULL);

    /* Is filename a valid string? */
    assert(filename != NULL);

    /* Reset structures for new load */
    DXF_RESET(dxf);

    /* Does file exist? */
    if(stat(filename, &statbuf) == -1) {
        SET_ERRNO_ERROR(dxf, dxfErrorInvalidFile);
        return 0;
    }

    /* Open file for reading */
    if((fd = open(filename, O_RDONLY)) == -1) {
        SET_ERRNO_ERROR(dxf, dxfErrorOpenFailed);
        return 0;
    }

    /* Load the DXF file */
    if(dxf_load_fd(dxf, fd) != 1) {
        return 0;
    }

    /* Close the file */
    if(close(fd) == -1) {
        SET_ERRNO_ERROR(dxf, dxfErrorCloseFailed);
        return 0;
    }

    return 1;
}

/**
Attempts to load a DXF from an open file descriptor.

@param  dxf DXF state structure.
@param  fd  File descriptor open and set to beginning of DXF stream.
@returns 1 on success, 0 on error.
*/
int dxf_load_fd(dxf_t *dxf, int fd) {
    FILE *fp;   /* file pointer for stream functions */
    int state = 0;
    char cur_section[DXF_MAX_LINE_LENGTH + 1];

    /* Is the structure valid? */
    assert(dxf != NULL);

    /* valid file descriptor? */
    if(fcntl(fd, F_GETFL) == -1) {
        SET_ERRNO_ERROR(dxf, dxfErrorBadFd);
        return 0;
    }

    /* Reset structures for new load */
    DXF_RESET(dxf);

    /* Get a stream pointer from fd */
    if((fp = fdopen(fd, "r")) == NULL) {
        SET_ERRNO_ERROR(dxf, dxfErrorBadFd);
        return 0;
    }

    /* Prepare the header variable dictionary */
    dxf->vars = sdict_create();
    assert(dxf->vars != NULL);

    /* Loop through and parse every DXF record */
    while(feof(fp) != 1) {
        int group_code = -1; /* Group code */
        char value[DXF_MAX_LINE_LENGTH + 1]; /* Value */

        /* Parse a record */
        value[0] = '\0';
        if(dxf_parse_record(dxf, fp, &group_code, value) == 0) {
            if(dxf->error.code == dxfErrorEOF) {
                return 1;
            }
            /* Error */
            return 0;
        }
        /*printf("cur_section=%s\n", cur_section);*/
        /*if(group_code == 9) {
            printf("%s\n", cur_section);
        }*/
        /*printf("%i: type=%i\n", group_code, dxf_group_type_map[group_code]);*/
        /*if(group_code == 0) {
            printf("%i: %s\n", group_code, value);
        }*/
        switch(state) {
            case 0:
                if((group_code == 0) && (strcmp(value, "SECTION") == 0)) {
                    state = 1;
                }
                break;
            case 1:
                if(group_code != 2) {
                    /* Error */
                    return 0;
                }
                if(snprintf(cur_section, sizeof(cur_section), "%s", value) !=
                    (int)strlen(value)) {
                    SET_ERROR(dxf, dxfErrorSnprintfFailed);
                    return 0;
                }
                if(strcasecmp(cur_section, "HEADER")) {
                }
                state = 2;
                break;
            case 2:
                if((group_code == 0) && (strcmp(value, "ENDSEC") == 0)) {
                    state = 0;
                    break;
                }
                /* Mid-section */
                if(!strcmp(cur_section, "HEADER")) {
                    if(group_code == 9) {
                        /* create a data element to add */
                        sdict_set(dxf->vars, value, NULL);
                    }
                }
                break;
        }
    }
    return 1;
}

/**
Get the last error.
Copies last error detail into provided structure.

@param  dxf DXF state structure.
@param  error DXF error structure.
@returns 1 on success, 0 on error.
*/
int dxf_get_last_error(dxf_t *dxf, dxf_error_t *error) {
    /* Is the structure valid? */
    assert(dxf != NULL);

    /* Is the error structure valid? */
    assert(error != NULL);

    /* Copy the current error into target structure */
    error->code = dxf->error.code;
    memcpy(error->msg, dxf->error.msg, sizeof(error->msg));

    return 1;
}

/**
Print last error details to stderr.
Convenience function that prints details of the last error to stderr.

@param  dxf DXF state structure.
*/
void dxf_print_last_error(dxf_t *dxf) {
    /* Is the structure valid? */
    assert(dxf != NULL);

    /* Print the error to stderr */
    fprintf(stderr, "DXF Error:\n" \
        "\tcode:\t%i\n" \
        "\tmsg:\t%s\n" \
        "\tline:\t%i\n" \
        "\tcolumn:\t%i\n",
        dxf->error.code, dxf->error.msg, dxf->line, dxf->column);
}

int dxf_has_var(dxf_t *dxf, const char *name) {
    /* Is the structure valid? */
    assert(dxf != NULL);

    /* Is the key valid? */
    assert((name != NULL) && (*name != '\0'));

    return sdict_has(dxf->vars, name);
}

