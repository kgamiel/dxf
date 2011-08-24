/** @file sdict.h 
 *  @brief String dictionary implementation.
 *
 * Unique string to void* dictionary.
 */
#ifndef _SDICT_H_
#define _SDICT_H_

/**
Dictionary node.
*/
typedef struct _sdict_node_t {
    char c;
    char terminal;
    struct _sdict_node_t *children;
    struct _sdict_node_t *next;
    struct _sdict_node_t *parent;
    void *value;
} sdict_node_t;

/*! @struct sdict_t
    @brief Unique String to void* Dictionary.
*/
typedef struct _sdict_t {
    sdict_node_t *top; /**< Top node pointer. */
} sdict_t;

/* API functions */
sdict_t *sdict_create();
int sdict_destroy(sdict_t *s);
int sdict_set(sdict_t *s, const char *key, void *value);
int sdict_get(sdict_t *s, const char *key, void **value);
int sdict_has(sdict_t *s, const char *key);
void sdict_print_node(sdict_node_t *n);

#endif

