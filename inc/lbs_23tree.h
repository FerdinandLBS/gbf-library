#ifndef _LBS_23TREE_H_
#define _LBS_23TREE_H_

#include "lbs_rwlock.h"

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

#ifdef LBS_TREE_MT_SAFE
    lbs_rwlock_t lock;
#endif
    
    /* Root node */
    lbs_23tree_node_t* root;
}lbs_23tree_t;

typedef enum _lbs_tree_status {
    LBS_TREE_STATUS_OK = 0,
    LBS_TREE_INV_PARAM = 1,
    LBS_TREE_NO_MEMORY = 2,
    LBS_TREE_ALREADY_EXIST = 3,
    LBS_TREE_NOT_FOUND = 4,
    LBS_TREE_INV_KEY = 5,
    LBS_TREE_EMPTY_TREE = 6,
    LBS_TREE_INTERNAL,
}lbs_tree_status_t;


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
    lbs_tree_status_t lbs_23tree_insert(
        lbs_23tree_t* tree,
        void* value,
        lbs_23tree_key_t key
        );

    /**
     * Search a key in tree and get a value
     */
    lbs_tree_status_t lbs_23tree_search(
        lbs_23tree_t* tree,
        lbs_23tree_key_t key,
        void** value
        );

    /**
     * Delete a key in tree
     */
    lbs_tree_status_t lbs_23tree_delete(
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

