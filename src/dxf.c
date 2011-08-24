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

/* Max line length according to DXF manual, not including NL */
#define DXF_MAX_LINE_LENGTH 2049

/* Local prototypes */
static dxf_error_t _dxf_load_fd(const dxf_handle_t dxf, int fd);

/* Local structures */

typedef struct _var_t {
    char *name;
    int type;
    union {
        char *c;
        double d;
        int i;
    } value;
} var_t;

typedef struct _section_t {
    int start;
    int end;
    char *name;
} section_t;

/*! @struct dxf_t
    @brief DXF state information.

    Maintains internal processing state.
*/
typedef struct _dxf_t {
    dxf_error_detail_t error; /**< Last error */
    int line; /**< Current DXF line number being processed */
    int column; /**< Current DXF column number being processed */
    char filename[FILENAME_MAX]; /**< Filename, if available */
    section_t *section; /**< Sections */
    int section_cnt;
    var_t *variable;
    int variable_cnt;
} dxf_t;

static void _dxf_add_variable(dxf_t *dxf, const char *name, int type, 
    const char *value) {
    assert(dxf != NULL);
    assert((name != NULL) && (*name != '\0'));
    /*assert((value != NULL) && (*value != '\0'));*/

    dxf->variable = (var_t*)realloc(dxf->variable, (sizeof(var_t) * 
        (dxf->variable_cnt + 1)));
    dxf->variable[dxf->variable_cnt].name = strdup(name);
    dxf->variable[dxf->variable_cnt].type = type;
    /* do the right thing based on type, this is char* example */
    if((value != NULL) && (*value != '\0')) {
        dxf->variable[dxf->variable_cnt].value.c = strdup(value);
    } else {
        dxf->variable[dxf->variable_cnt].value.c = strdup("NA");
    }
    dxf->variable_cnt++;
}

#define MAX_OPEN_DXF 1024

/* This should be optimized */
/* Internal map of handles to dxf_t structures.
*/
static dxf_t **g_handle_to_dxf = (dxf_t**)NULL; 
static short g_init = 0;

static void _dxf_init() {
    if(g_init != 1) {
        g_handle_to_dxf = (dxf_t**)calloc(MAX_OPEN_DXF, sizeof(dxf_t*));
        assert(g_handle_to_dxf != NULL);
        g_init = 1;
    }
}

static void _dxf_cleanup() {
    if(g_init == 1) {
        dxf_handle_t handle;
        for(handle = 0; handle < MAX_OPEN_DXF; handle++) {
            if(g_handle_to_dxf[handle] != NULL) {
                return;
            }
        }
        free(g_handle_to_dxf);
        g_handle_to_dxf = (dxf_t**)NULL;
        g_init = 0;
    }
}

extern int dxf_group_type_map[];

static dxf_error_t dxf_get_registered(const dxf_handle_t handle,
    dxf_t **dxf) {
    if(g_handle_to_dxf[handle] != NULL) {
        *dxf = g_handle_to_dxf[handle];
        return dxfErrorOk;
    }
    return dxfErrorInvalidHandle;
}

static dxf_error_t dxf_unregister_handle(const dxf_handle_t handle) {
    dxf_error_t err;
    dxf_t *dxf;
    int i;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    if(dxf->section != NULL) {
        for(i = 0; i < dxf->section_cnt; i++) {
            free(dxf->section[i].name);
        }
        free(dxf->section);
    }

    if(dxf->variable != NULL) {
        for(i = 0; i < dxf->variable_cnt; i++) {
            free(dxf->variable[i].name);
            if(dxf->variable[i].value.c != NULL) {
                free(dxf->variable[i].value.c);
            }
        }
        free(dxf->variable);
    }

    free(dxf);
    g_handle_to_dxf[handle] = (dxf_t*)NULL;
    return dxfErrorOk;
}

static dxf_error_t dxf_register_handle(dxf_handle_t *handle, dxf_t **dxf) {
    assert(dxf != NULL);
    for((*handle) = 0; (*handle) < MAX_OPEN_DXF; ((*handle)++)) {
        if(g_handle_to_dxf[(*handle)] == NULL) {
            (*dxf) = (dxf_t*)calloc(1, sizeof(dxf_t));
            assert((*dxf) != NULL);
            g_handle_to_dxf[(*handle)] = (*dxf);
            return dxfErrorOk;
        }
    }

    return dxfErrorTooManyOpen;
}

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
    "Snprintf failed",
    "Too many open DXF files",
    "Invalid DXF handle"
};

dxf_error_t dxf_print_error(const dxf_error_t code, FILE *fp) {
    fprintf(fp, "%s", DXF_ERROR_S[code]);
    return dxfErrorOk;
}

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

const char *dxf_type_enum_to_name(const int type) {
    if(type == 0) {
        return "dxfString2049";
    } else if(type == 1) {
        return "dxfDouble";
    } else if(type == 2) {
        return "dxfInt16";
    } else if(type == 3) {
        return "dxfInt32";
    } else if(type == 4) {
        return "dxfString255";
    } else if(type == 5) {
        return "dxfInt64";
    } else if(type == 6) {
        return "dxfBoolean";
    } else if(type == 7) {
        return "dxfLong";
    } else {
        fprintf(stderr, "Invalid group type: %i\n", type);
        exit(EXIT_FAILURE);
    }
}

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
        } else if(isascii((int)line[i]) == 0) {
            SET_ERROR(dxf, dxfErrorNonASCII);
            return 0;
        }

        /* If id, must be all digits */
        if((id == 1) && (isdigit(line[i]) == 0)) {
            SET_ERROR(dxf, dxfErrorDigitExpected);
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

@param  handle  DXF handle.
@param  filename    Filename.
@returns On success, handle will contain a valid handle for use in future API
calls and dxfErrorOk is returned.  On failure, handle is undefined and a
relevant error code is returned.
*/
dxf_error_t dxf_load(dxf_handle_t *handle, const char *filename) {
    struct stat statbuf; /* Struct for stat() call */
    int fd; /* File descriptor */
    dxf_t *dxf;
    dxf_error_t err;

    _dxf_init();

    /* Make sure we have a non-NULL location for handle */
    assert(handle != NULL);

    /* Is filename non-NULL? */
    assert(filename != NULL);

    /* Does file exist? */
    if(stat(filename, &statbuf) == -1) {
        return dxfErrorInvalidFile;
    }

    /* Open file for reading */
    if((fd = open(filename, O_RDONLY)) == -1) {
        return dxfErrorOpenFailed;
    }

    /* Register a new handle and internal data structure */
    if((err = dxf_register_handle(handle, &dxf)) != dxfErrorOk) {
        (void)close(fd);
        return err;
    }

    /* Save filename */
    snprintf(dxf->filename, sizeof(dxf->filename), "%s", filename);

    /* Load the DXF file */
    if((err = _dxf_load_fd((*handle), fd)) != dxfErrorOk) {
        (void)close(fd);
        dxf_unload((*handle));
        return err;
    }

    /* Close the file */
    (void)close(fd);
    return dxfErrorOk;
}

/**
Attempts to load a DXF from an open file descriptor.

@param  dxf DXF state structure.
@param  fd  File descriptor open and set to beginning of DXF stream.
@returns dxfErrorOk on success, 0 on error.
*/
static dxf_error_t _dxf_load_fd(const dxf_handle_t handle, int fd) {
    FILE *fp;   /* file pointer for stream functions */
    char cur_section[DXF_MAX_LINE_LENGTH + 1];
    dxf_t *dxf;
    dxf_error_t err;
    enum { S_PRE_SECTION, S_START_SECTION, S_SECTION, S_HEADER_VALUE }
        state = S_PRE_SECTION;
    int section_start, section_end;
    char buf0[DXF_MAX_LINE_LENGTH + 1];

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    /* valid file descriptor? */
    if(fcntl(fd, F_GETFL) == -1) {
        SET_ERRNO_ERROR(dxf, dxfErrorBadFd);
        return dxf->error.code;
    }

    /* Get a stream pointer from fd */
    if((fp = fdopen(fd, "r")) == NULL) {
        SET_ERRNO_ERROR(dxf, dxfErrorBadFd);
        return dxf->error.code;
    }

    /* Loop through and parse every DXF record */
    while(feof(fp) != 1) {
        int group_code = -1; /* Group code */
        char value[DXF_MAX_LINE_LENGTH + 1]; /* Value */

        /* Parse a record */
        value[0] = '\0';
        if(dxf_parse_record(dxf, fp, &group_code, value) == 0) {
            if(dxf->error.code == dxfErrorEOF) {
                return dxfErrorOk;
            }
            /* Error */
            return dxf->error.code;
        }
        /*
        printf("cur_section=%s\n", cur_section);
        if(group_code == 9) {
            printf("%s\n", cur_section);
        }
        printf("%i: type=%i\n", group_code, dxf_group_type_map[group_code]);
        if(group_code == 0) {
            printf("%i: %s\n", group_code, value);
        }
        */
        switch(state) {
            case S_PRE_SECTION:
                if(group_code != 0) {
                    fprintf(stderr, "KAG: Expected group code 0 at %i\n",
                        dxf->line);
                    return dxfErrorInvalidFormat;
                }
                if(strcmp(value, "SECTION") == 0) {
                    state = S_START_SECTION;
                } else if(strcmp(value, "EOF") == 0) {
                    return dxfErrorOk;
                } else {
                    fprintf(stderr, "KAG: Expected SECTION or EOF at %i\n",
                        dxf->line);
                    return dxfErrorInvalidFormat;
                }
                break;
            case S_START_SECTION:
                if(group_code != 2) {
                    fprintf(stderr, "KAG: Expected group code 2 at %i\n",
                        dxf->line);
                    return dxfErrorInvalidFormat;
                }
                if(snprintf(cur_section, sizeof(cur_section), "%s", value) !=
                    (int)strlen(value)) {
                    SET_ERROR(dxf, dxfErrorSnprintfFailed);
                    return dxf->error.code;
                }
                section_start = dxf->line;
                state = S_SECTION;
                break;
            case S_SECTION:
                if((group_code == 0) && (strcmp(value, "ENDSEC") == 0)) {
                    state = S_PRE_SECTION;
                    section_end = dxf->line;
                    dxf->section = (section_t*)realloc(dxf->section,
                        (sizeof(section_t) * (dxf->section_cnt + 1)));
                    dxf->section[dxf->section_cnt].start = section_start;
                    dxf->section[dxf->section_cnt].end = section_end;
                    dxf->section[dxf->section_cnt].name = strdup(cur_section);
                    dxf->section_cnt++;
                    break;
                }
                /* Mid-section */
                if(strcmp(cur_section, "HEADER") == 0) {
                    if(group_code == 9) {
                        /* create a data element to add */
                        /* value contains a header variable */
                        /*printf("HEADER VAR = %s\n", value);*/
                        strcpy(buf0, value);
                        state = S_HEADER_VALUE;
                    }
                }
                break;
            case S_HEADER_VALUE:
                _dxf_add_variable(dxf,  buf0, group_code, value);
                state = S_SECTION;
                break;
        }
    }
    return dxfErrorOk;
}

/**
Get the last error.
Copies last error detail into provided structure.

@param  dxf DXF state structure.
@param  error DXF error structure.
@returns 1 on success, 0 on error.
*/
dxf_error_t dxf_get_last_error(const dxf_handle_t handle, dxf_error_detail_t
    *error) {
    dxf_t *dxf;
    dxf_error_t err;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    /* Is the error structure valid? */
    assert(error != NULL);

    /* Copy the current error into target structure */
    error->code = dxf->error.code;
    memcpy(error->msg, dxf->error.msg, sizeof(error->msg));

    return dxfErrorOk;
}

/**
Print last error details to stderr.
Convenience function that prints details of the last error to stderr.

@param  dxf DXF state structure.
*/
dxf_error_t dxf_print_last_error(const dxf_handle_t handle) {
    dxf_t *dxf;
    dxf_error_t err;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    /* Print the error to stderr */
    fprintf(stderr, "DXF Error:\n" \
        "\tcode:\t%i\n" \
        "\tmsg:\t%s\n" \
        "\tline:\t%i\n" \
        "\tcolumn:\t%i\n",
        dxf->error.code, dxf->error.msg, dxf->line, dxf->column);

    return dxfErrorOk;
}

dxf_error_t dxf_has_var(const dxf_handle_t handle, const char *name) {
    dxf_t *dxf;
    dxf_error_t err;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    /* Is the key valid? */
    assert((name != NULL) && (*name != '\0'));

    return dxfErrorInvalidVariable;
}

/**
Unload resources and free the handle.

@param  dxf dxf handle.
@returns dxfErrorOk on success, error code otherwise.
*/
dxf_error_t dxf_unload(const dxf_handle_t handle) {
    dxf_t *dxf;
    dxf_error_t err;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    err = dxf_unregister_handle(handle);
    _dxf_cleanup();

    return err;
}

dxf_error_t dxf_print(dxf_handle_t handle, FILE *fp) {
    dxf_t *dxf;
    dxf_error_t err;
    int i;

    if((err = dxf_get_registered(handle, &dxf)) != dxfErrorOk) {
        return err;
    }

    fprintf(fp, "%s\n", dxf->filename);
    fprintf(fp, "\tlines: %i\n", dxf->line);
    for(i = 0; i < dxf->section_cnt; i++) {
        fprintf(fp, "\t%s (%i - %i)\n", 
            dxf->section[i].name,
            dxf->section[i].start,
            dxf->section[i].end);
    }
    for(i = 0; i < dxf->variable_cnt; i++) {
        fprintf(fp, "\t%s\t", dxf_type_enum_to_name(
            dxf_group_type_map[dxf->variable[i].type]));
        fprintf(fp, "%s = ",
            dxf->variable[i].name);
        if(dxf->variable[i].value.c != NULL) {
            fprintf(fp, "%s", dxf->variable[i].value.c);
        }
        fprintf(fp, "\n");
    }

    return dxfErrorOk;
}

