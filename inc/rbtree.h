#ifndef _RBTREE_H_
#define _RBTREE_H_

#include <stdlbs.h>

/**
 * Customized Red-black Binary Tree
 *
 *  1) A node is either red or black
 *  2) The root is black
 *  3) All leaves (NULL) are black
 *  4) Both children of every red node are black
 *  5) Every simple path from root to leaves contains the same number
 *     of black nodes.
 * !6) Only left link would be red
 *
 * Ferdinand 2017/10/18
 */

typedef struct __rbtree_node_st_ {
    /* 0000 0000 
     *      ||||
     *      |||`- color
     *      ||`-- left child color
     *      |`- is root
     *      `- is left node
     * COLOR: 0 -- black, 1 -- red
     */
    char status;

    /* Key-value pair */
    int key;
    void* data;

    struct __rbtree_node_st_* parent;
    struct __rbtree_node_st_* lnode;
    struct __rbtree_node_st_* rnode;
}rbtree_node_t;

typedef struct __rbtree_st_ {
    unsigned depth;
    unsigned count;

    /* reference to RBT_MODE_XXX */
    unsigned mode;

    unsigned pool_size;
    rbtree_node_t* pool;

    rbtree_node_t* root;
}rbtree_t;

/* rb-tree running mode */
#define RBT_MODE_INTEGRITY  0x00000001
//#define RBT_MODE_MUL_THREAD 0x00000002


/* rb-tree status code */
#define RBT_CODE_OK            0 // Success
#define RBT_CODE_INV_PARAM     1 // Parameters error
#define RBT_CODE_NO_MEMORY     2 // No memory
#define RBT_CODE_ALREADY_EXSIT 3 // Key already exists
#define RBT_CODE_NOT_FOUND     4 // Key not found
#define RBT_CODE_CORRUPT       5 // Data corrupt

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * create a new tree
     * @output: 2nd rank pointer of tree
     * @input: node count cached
     * @input: running mode
     * @return: status
     */
    int rbt_new_tree(rbtree_t** tree, unsigned init_pool_size, unsigned mode);

    /**
     * insert a key-value pair
     * @input: pointer of tree
     * @input: key
     * @input: data
     * @input: update data if exists
     * @inputL old data, if update
     * @return: status
     */
    int rbt_insert(rbtree_t* tree, int key, void* data, lbs_bool_t is_update, void** old_data);

    /**
     * find a node by key
     * @input: pointer of tree
     * @input: key
     * @return: pointer of node
     */
    rbtree_node_t* rbt_search(rbtree_t* tree, int key);

    /**
     * find first node which contain the smallest value
     * @input: pointer of root node
     * @return: pointer of target node
     */
    rbtree_node_t* rbt_first(rbtree_node_t* root);

    /**
     * find last node which contain the smallest value
     * @input: pointer of root node
     * @return: pointer of target node
     */
    rbtree_node_t* rbt_last(rbtree_node_t* root);
    
    /**
     * find next node after specified node.
     * @input: pointer of current node
     * @return: pointer of target node
     *          return ZERO if there is no other node contain bigger key
     */
    rbtree_node_t* rbt_next(rbtree_node_t* current);
    
    /**
     * find previous node before specified node.
     * @input: pointer of current node
     * @return: pointer of target node
     *          return ZERO if there is no other node contain bigger key
     */
    rbtree_node_t* rbt_prev(rbtree_node_t* current);

    /**
     * remove a node by key
     * @input: pointer of tree
     * @input: key
     * @output: data referenced with specifeid key
     * @return: status
     */
    int rbt_remove(rbtree_t* tree, int key, void** data);

    /**
     * delete a tree. all data will be lost
     * @intput: pointer of tree
     */
    void rbt_delete_tree(rbtree_t* tree);

#ifdef __cplusplus
};
#endif

#endif
