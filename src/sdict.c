#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "sdict.h"
#include "util.h"

/**
Create a new node.

@param  c   Node character.
@returns Pointer on success, NULL on failure.
*/
static sdict_node_t *sdict_node_create(char c) {
    sdict_node_t *n = NULL;

    n = (sdict_node_t*)calloc(1, sizeof(sdict_node_t));
    assert(n != NULL);
    n->c = c;
    return n;
}

/**
Create a new dictionary.
*/
sdict_t *sdict_create() {
    sdict_t *s = NULL;

    s = (sdict_t*)calloc(1, sizeof(sdict_t));
    assert(s != NULL);
    assert(s->top == NULL); /* satisfies splint */
    s->top = sdict_node_create('.');
    assert(s->top != NULL);

    return s;
}

static void sdict_node_destroy(sdict_node_t *n) {
    sdict_node_t *cn;

    if(n == NULL) {
        return;
    }
    for(cn = n; cn != NULL; ) {
        sdict_node_t *x = NULL;
        if(cn->children != NULL) {
            sdict_node_destroy(cn->children);
        }
        x = cn->next;
        assert(cn != NULL);
        free(cn);
        cn = x;
    }
}

int sdict_destroy(sdict_t *s) {
    assert(s != NULL);
    sdict_node_destroy(s->top);
    assert(s != NULL);
    free(s);

    return 1;
}

static int sdict_node_set(sdict_node_t *n, const char *key, int key_len, int
    index, char *value) {
    char c;
    sdict_node_t *cur_node = n;

    assert(n != NULL);
    assert(key != NULL);
    assert(key_len > 0);
    assert((index >= 0) && (index < key_len));

    /* Get current character to be updated or added at this level */
    c = key[index];

    /* Add current child under n */
    if(n->children == NULL) {
        /* First child of n */
        n->children = sdict_node_create(c);
        n->children->parent = n;
        cur_node = n->children;
    } else {
        /* Not first child, scan list for matching child */
        for(cur_node = n->children; cur_node; cur_node = cur_node->next) {
            if(cur_node->c == c) {
                /* We already have a node with this value */
                break;
            }
            if(cur_node->next == NULL) {
                /* There is no node at this level, add it */
                cur_node = cur_node->next = sdict_node_create(c);
                break;
            }
        }
    }
    cur_node->parent = n;
    /* Are we done with this key? */
    if(key_len > (index + 1)) {
        /* No, more chars in key */
        return sdict_node_set(cur_node, key, key_len, (index + 1),
            value);
    } else {
        /* Yes, set the value and return */
        cur_node->value = value;
        cur_node->terminal = (char)1;
        return 1;
    }
}

int sdict_set(sdict_t *s, const char *key, void *value) {
    int index = 0;

    assert(s != NULL);
    assert(key != NULL);
    assert((*key) != '\0');

    return sdict_node_set(s->top, key, (int)strlen(key), index, value);
}

/**
@returns 
*/
/*@null@*/
static sdict_node_t *sdict_node_get(sdict_node_t *n, const char *key,
    int key_len, int index) {
    char c;
    sdict_node_t *cur_node = n;

    assert(n != NULL);
    assert(key != NULL);
    assert(key_len > 0);
    assert((index >= 0) && (index < key_len));

    if(n->children == NULL) {
        return ((sdict_node_t*)NULL);
    } else {
        c = key[index];
        for(cur_node = n->children;cur_node!= NULL; cur_node = cur_node->next) {
            if((index + 1) == key_len) {
                if((cur_node->c == c) && (cur_node->terminal == (char)1)) {
                    /* Found it */
                    return cur_node;
                } else {
                    /* Not found */
                    return NULL;
                }
            } else {
                /* Keep looking */
                return sdict_node_get(cur_node, key, key_len, index + 1);
            }
        }
    }
    return NULL;
}

/**
@returns 1 if key exists and value points to item (which could be NULL),
otherwise 0.
*/
int sdict_get(sdict_t *s, const char *key, void **value) {
    sdict_node_t *n = NULL;
    assert(s != NULL);
    assert(key != NULL);

    if((n = sdict_node_get(s->top, key, (int)strlen(key), 0)) != NULL) {
        *value = n->value;
        return 1;
    }
    return 0;
}

int sdict_has(sdict_t *s, const char *key) {
    sdict_node_t *n;

    assert(s != NULL);
    assert(key != NULL);

    if((n = sdict_node_get(s->top, key, (int)strlen(key), 0)) != NULL) {
        return (int)n->terminal;
    } 
    return 0;
}

void sdict_print(sdict_node_t *n, char *value, int value_len) {
    sdict_node_t *x;
    char s[8192];
    int i = sizeof(s) - 1;
    memset(s, 0x20, sizeof(s));
    for(x=n; x->parent != NULL;x = x->parent) {
        s[i--] = x->c;
    }
    util_trim(s);
    snprintf(value, value_len, "%s", s);
}

void sdict_print_node(sdict_node_t *n) {
    sdict_node_t *cur_node = n;

    assert(n != NULL);

    if(n->children == NULL) {
        return;
    } else {
        for(cur_node = n->children;cur_node!= NULL; cur_node = cur_node->next) {
            if(cur_node->terminal == (char)1) {
                char buf[8192];
                sdict_print(cur_node, buf, sizeof(buf));
                printf("%s\n", buf);
            } else {
                /* Keep looking */
                sdict_print_node(cur_node);
            }
        }
    }
}
