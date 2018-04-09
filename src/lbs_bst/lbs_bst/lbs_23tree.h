#ifndef _LBS_23TREE_H_
#define _LBS_23TREE_H_

#include <lbs_errno.h>

typedef unsigned long lbs_23tree_key_t;
typedef struct _lbs_23tree_node_st lbs_23tree_node_t;
struct _lbs_23tree_node_st {
    int isroot;

    lbs_23tree_node_t* parent;
    lbs_23tree_node_t* lchild;
    lbs_23tree_node_t* mchild;
    lbs_23tree_node_t* mchild2;
    lbs_23tree_node_t* rchild;

    lbs_23tree_key_t lkey;
    void* lvalue;
    lbs_23tree_key_t rkey;
    void* rvalue;
};

typedef struct _lbs_23tree_st {
    /* Key value counts */
    int node_count;
    
    /* The depth of tree */
    int level;
    
    /* Root node */
    lbs_23tree_node_t* root;
}lbs_23tree_t;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Create new tree
     */
    lbs_23tree_t* lbs_23tree_new_tree();

    /**
     * Insert a key value into tree
     */
    lbs_status_t lbs_23tree_insert(
        lbs_23tree_t* tree,
        void* value,
        lbs_23tree_key_t key
        );

    /**
     * Search a key in tree and get a value
     */
    lbs_status_t lbs_23tree_search(
        lbs_23tree_t* tree,
        lbs_23tree_key_t key,
        void** value
        );

    /**
     * Delete a key in tree
     */
    lbs_status_t lbs_23tree_delete(
        lbs_23tree_t* tree,
        lbs_23tree_key_t key
        );

    /**
     * Free a tree. All data in it will be lost.
     */
    void lbs_23tree_release(
        lbs_23tree_t* tree
        );

#ifdef __cplusplus
};
#endif

#endif //_LBS_23TREE_H_

