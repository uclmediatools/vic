/*
 * FILE:    btree.c
 * PROGRAM: RAT
 * AUTHOR:  O.Hodson
 * 
 * Binary tree implementation - Mostly verbatim from:
 *
 * Introduction to Algorithms by Corman, Leisserson, and Rivest,
 * MIT Press / McGraw Hill, 1990.
 *
 */

#include "config_unix.h"
#include "config_win32.h"

#include "debug.h"
#include "memory.h"
#include "btree.h"

typedef struct s_btree_node {
        u_int32       key;
        void         *data;
        struct s_btree_node *parent;
        struct s_btree_node *left;
        struct s_btree_node *right;
} btree_node_t;

struct s_btree {
        btree_node_t   *root;
        int             cnt;
};

/*****************************************************************************/
/* Utility functions                                                         */
/*****************************************************************************/

static btree_node_t*
btree_min(btree_node_t *x)
{
        if (x == NULL) {
                return NULL;
        }
        while(x->left) {
                x = x->left;
        }
        return x;
}

static btree_node_t*
btree_max(btree_node_t *x)
{
        if (x == NULL) {
                return NULL;
        }
        while(x->right) {
                x = x->right;
        }
        return x;
}

static btree_node_t*
btree_successor(btree_node_t *x)
{
        btree_node_t *y;

        if (x->right != NULL) {
                return btree_min(x->right);
        }

        y = x->parent;
        while (y != NULL && x == y->right) {
                x = y;
                y = y->parent;
        }

        return y;
}

static btree_node_t*
btree_search(btree_node_t *x, u_int32 key)
{
        while (x != NULL && key != x->key) {
                if (key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }
        return x; 
}

static void
btree_insert_node(btree_t *tree, btree_node_t *z) {
        btree_node_t *x, *y;

        y = NULL;
        x = tree->root;
        while (x != NULL) {
                y = x;
                assert(z->key != x->key);
                if (z->key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }

        z->parent = y;
        if (y == NULL) {
                tree->root = z;
        } else if (z->key < y->key) {
                y->left = z;
        } else {
                y->right = z;
        }
}

static btree_node_t*
btree_delete_node(btree_t *tree, btree_node_t *z)
{
        btree_node_t *x, *y;

        if (z->left == NULL || z->right == NULL) {
                y = z;
        } else {
                y = btree_successor(z);
        }

        if (y->left != NULL) {
                x = y->left;
        } else {
                x = y->right;
        }

        if (x != NULL) {
                x->parent = y->parent;
        }

        if (y->parent == NULL) {
                tree->root = x;
        } else if (y == y->parent->left) {
                y->parent->left = x;
        } else {
                y->parent->right = x;
        }

        z->key  = y->key;
        z->data = y->data;

        return y;
}

/*****************************************************************************/
/* Exported functions                                                        */
/*****************************************************************************/

int
btree_create(btree_t **tree)
{
        btree_t *t = (btree_t*)xmalloc(sizeof(btree_t));
        if (t) {
                t->root = NULL;
                t->cnt  = 0;
                *tree = t;
                return TRUE;
        }
        return FALSE;
}

int
btree_destroy(btree_t **tree)
{
        btree_t *t = *tree;

        if (t->root != NULL) {
                debug_msg("Tree not empty - cannot destroy\n");
                return FALSE;
        }

        xfree(t);
        *tree = NULL;
        return TRUE;
}

int
btree_find(btree_t *tree, u_int32 key, void **d)
{
        btree_node_t *x;
        x = btree_search(tree->root, key);
        if (x != NULL) {
                *d = x->data;
                return TRUE;
        }
        return FALSE;
}

int 
btree_add(btree_t *tree, u_int32 key, void *data)
{
        btree_node_t *x;

        x = btree_search(tree->root, key);
        if (x != NULL) {
                debug_msg("Item already exists - key %ul\n", key);
                return FALSE;
        }

        x = (btree_node_t *)xmalloc(sizeof(btree_node_t));
        x->key  = key;
        x->data = data;
        x->parent = x->left = x->right = NULL;
        btree_insert_node(tree, x);
        tree->cnt++;

        return TRUE;
}

int
btree_remove(btree_t *tree, u_int32 key, void **data)
{
        btree_node_t *x;

        x = btree_search(tree->root, key);
        if (x == NULL) {
                debug_msg("Item not on tree - key %ul\n", key);
                *data = NULL;
                return FALSE;
        }

        /* Note value that gets freed is not necessarily the the same
         * as node that gets removed from tree since there is an
         * optimization to avoid pointer updates in tree which means
         * sometimes we just copy key and data from one node to
         * another.  
         */

        *data = x->data;
        x = btree_delete_node(tree, x);
        xfree(x);
        tree->cnt--;

        return TRUE;
}

int 
btree_get_min_key(btree_t *tree, u_int32 *key)
{
        btree_node_t *x;

        if (tree->root == NULL) {
                return FALSE;
        }

        x = btree_min(tree->root);
        if (x == NULL) {
                return FALSE;
        }
        
        *key = x->key;
        return TRUE;
}

int 
btree_get_max_key(btree_t *tree, u_int32 *key)
{
        btree_node_t *x;

        if (tree->root == NULL) {
                return FALSE;
        }

        x = btree_max(tree->root);
        if (x == NULL) {
                return FALSE;
        }
        
        *key = x->key;
        return TRUE;
}

int
btree_get_next_key(btree_t *tree, u_int32 cur_key, u_int32 *next_key)
{
        btree_node_t *x;

        x = btree_search(tree->root, cur_key);
        if (x == NULL) {
                return FALSE;
        }
        
        x = btree_successor(x);
        if (x == NULL) {
                return FALSE;
        }
        
        *next_key = x->key;
        return TRUE;
}

int 
btree_get_element_count(btree_t *tree)
{
        return tree->cnt;
}

/*****************************************************************************/
/* Test code                                                                 */
/*****************************************************************************/

#ifdef TEST_BTREE

static int
btree_depth(btree_node_t *x)
{
        int l, r;

        if (x == NULL) {
                return 0;
        }

        l = btree_depth(x->left);
        r = btree_depth(x->right);

        if (l > r) {
                return l + 1;
        } else {
                return r + 1;
        }
}

#include <curses.h>

static void
btree_dump_node(btree_node_t *x, int depth, int c, int w)
{
        if (x == NULL) {
                return;
        }
        
        move(depth * 2, c);
        printw("%lu", x->key);
        refresh();

        btree_dump_node(x->left,  depth + 1, c - w/2, w/2);
        btree_dump_node(x->right, depth + 1, c + w/2, w/2);

        return;
}

static void
btree_dump(btree_t *b)
{
        initscr();
        btree_dump_node(b->root, 0, 40, 48);
        refresh();
        endwin();
}

#include "stdlib.h"

int 
main()
{
        btree_t *b;
        u_int32 i, *x;
        u_int32 v[] = {15, 5, 16, 3, 12, 20, 10, 13, 18, 23, 6, 7}; 
        u_int32 nv = sizeof(v) / sizeof(v[0]);

        btree_create(&b);

        for(i = 0; i < nv; i++) {
                x = (u_int32*)xmalloc(sizeof(u_int32));
                *x = (u_int32)random();
                if (btree_add(b, v[i], (void*)x) != TRUE) {
                        printf("Fail Add %lu %lu\n", v[i], *x);
                }
        }
    
        printf("depth %d\n", btree_depth(b->root));
        btree_dump(b);

        sleep(3);
        btree_remove(b, 5, (void*)&x);
        btree_dump(b);
        sleep(3);
        btree_remove(b, 16, (void*)&x);
        btree_dump(b);
        sleep(3);
        btree_remove(b, 13, (void*)&x);
        btree_dump(b);

        while (btree_get_root_key(b, &i)) {
                if (btree_remove(b, i, (void*)&x) == FALSE) {
                        fprintf(stderr, "Failed to remove %lu\n", i);
                }
                btree_dump(b);
                sleep(1); 
        }

        if (btree_destroy(&b) == FALSE) {
                printf("Failed to destroy \n");
        }
                
        return 0;
}

#endif /* TEST_BTREE*/


