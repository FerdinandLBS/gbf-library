#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lbs_23tree.h"

#ifndef INTERNALCALL
#define INTERNALCALL static
#endif
#define INVALID_KEY_VALUE (unsigned long)(-1)

typedef enum lbs_23tree_search_result {
   SEARCH_RES_LVALUE = 0,
   SEARCH_RES_RVALUE = 1,
   SEARCH_RES_TARGET = 2,
   SEARCH_RES_PARENT = 3,
   SEARCH_RES_ERROR  = 4,
}lbs_tree_search_reuslt;

#define IS_LEAF_NODE(_node_) (_node_->lchild == 0)
#define IS_TRI_NODE(_node_) (_node_->lkey!=INVALID_KEY_VALUE && _node_->rkey!=INVALID_KEY_VALUE)
#define IS_TWO_NODE(_node_) (!IS_TRI_NODE(_node_))
#define IS_ROOT_NODE(_node_) (_node_->isroot == 1)
#define IS_LEFT_CHILD(_node_) (_node_ == _node_->parent->lchild)
#define IS_MID_CHILD(_node_) (_node_ == _node_->parent->mchild)
#define IS_RIGHT_CHILD(_node_) (_node_ == _node_->parent->rchild)
#define CHANGE_ROOT_NODE(_new_, _old_) do{\
    if (IS_ROOT_NODE(_old_)) {\
        *((void**)_old_->parent) = _new_;\
        if (_new_ != 0) {\
        _new_->parent=_old_->parent;\
        _new_->isroot=1;\
        }\
        _old_->isroot=0;\
    } else \
        assert(0);\
}while(0)
#define SET_LEFT_CHILD(_parent_,_child_) do{if (_parent_!=0){if(_child_!=0){_parent_->lchild = _child_;_child_->parent = _parent_;}}}while(0)
#define SET_MID_CHILD(_parent_,_child_) do{if (_parent_!=0){if(_child_!=0){_parent_->mchild = _child_;_child_->parent = _parent_;}}}while(0)
#define SET_RIGHT_CHILD(_parent_,_child_) do{if (_parent_!=0){if(_child_!=0){_parent_->rchild = _child_;_child_->parent = _parent_;}}}while(0)
#define SET_RELATIONS(_parent_, _lchild_, _mchild_, _rchild_) do{\
    if (_parent_ != 0) {\
        if (_lchild_ != 0) {\
            _parent_->lchild = _lchild_;\
            _lchild_->parent = _parent_;\
        }\
        if (_mchild_ != 0) {\
            _parent_->mchild = _mchild_;\
            _mchild_->parent = _parent_;\
        }\
        if (_rchild_ != 0) {\
            _parent_->rchild = _rchild_;\
            _rchild_->parent = _parent_;\
        }\
    }\
}while(0)
#define CLR_LEFT_CHILD(_node_) (_node_->lchild = 0)
#define CLR_MID_CHILD(_node_) (_node_->mchild = 0)
#define CLR_RIGHT_CHILD(_node_) (_node_->rchild = 0)

#define SET_RKEY_VALUE(_node_,_rk_,_rv_) do{_node_->rkey=_rk_;_node_->rvalue=_rv_;}while(0)
#define SET_LKEY_VALUE(_node_,_lk_,_lv_) do{_node_->lkey=_lk_;_node_->lvalue=_lv_;}while(0)
#define CLR_RKEY_VALUE(_node_) SET_RKEY_VALUE(_node_, INVALID_KEY_VALUE, 0)
#define CLR_LKEY_VALUE(_node_) SET_LKEY_VALUE(_node_, INVALID_KEY_VALUE, 0)
#define INIT_KEY_VALUE(_node_,_lk_,_lv_,_rk_,_rv_) do{_node_->lkey=_lk_;_node_->lvalue=_lv_;_node_->rkey=_rk_;_node_->rvalue=_rv_;}while(0)
#define SWAP_KEY_VALUE(A,B) do{(long)A=(long)A^(long)B;(long)B=(long)A^(long)B;(long)A=(long)A^(long)B;}while(0)
#define SWAP_NODE_VALUE(A,B) SWAP_KEY_VALUE(A->lkey, B->lkey);\
    SWAP_KEY_VALUE(A->lvalue, B->lvalue);\
    SWAP_KEY_VALUE(A->rkey, B->rkey);\
    SWAP_KEY_VALUE(A->rvalue, B->rvalue);
#define CMP_KEY_VALUE(_node_, _key_, _result_) do{if (_node_->lkey==_key_)_result_=SEARCH_RES_LVALUE;else if (_node_->rkey==_key_)_result_=SEARCH_RES_RVALUE;else _result_=SEARCH_RES_PARENT;}while(0)

#define GET_LEVEL_POINTER(_root_) (int*)((long)_root_+(long)&lbs_23tree_internal_tree.level-(long)&lbs_23tree_internal_tree.root)
#define GET_PARENT_NODE(_node_) (IS_ROOT_NODE(_node_)?0:_node_->parent)
#define GET_BRO_NODE(_node_) (IS_LEFT_CHILD(_node)?(IS_TRI_NODE(_node_->parent)?_node_->parent->rchild:_node_->parent->rchild):(IS_MID_CHILD(_node_)?_node_->parent->lchild:(IS_TRI_NODE(_node_->parent)?_node_->parent->mchild:_node_->parent->lchild)))
#define GET_POSIBLE_SUBTREE(_node_, _key_) ((_node_->lkey>_key_)?(_node_->lchild):(IS_TRI_NODE(_node_)?((_node_->rkey<_key_)?_node_->rchild:_node_->mchild):(_node_->rchild)))

lbs_23tree_node_t* null_node = 0;
lbs_23tree_t lbs_23tree_internal_tree;


/**
 * Create new tree object
 * Nothing special, easy to use
 */
lbs_23tree_t* lbs_23tree_new_tree()
{
    lbs_23tree_t* tree = (lbs_23tree_t*)calloc(1, sizeof(lbs_23tree_t));
    
    if (tree == 0) {
        return 0;
    }

    return tree;
}

/**
 * Initialize tree data
 * @description:
 *     null
 * @input: pointer of tree
 * @return: null
 */
void lbs_23tree_init(lbs_23tree_t* tree)
{
    memset(tree, 0, sizeof(lbs_23tree_t));
}

/**
 * Create new tree node
 * @description:
 *     null
 * @input: value
 * @input: key
 * @return: pointer of node
 */
INTERNALCALL lbs_23tree_node_t* lbs_23tree_new_node(
    void* value, 
    lbs_23tree_key_t key
    )
{
    lbs_23tree_node_t* node = (lbs_23tree_node_t*)calloc(1 ,sizeof(lbs_23tree_node_t));

    if (node == 0) {
        return 0;
    }

    INIT_KEY_VALUE(node, key, value, INVALID_KEY_VALUE, 0);
    
    return node;
}

/**
 * Handle the null leaf
 * @description:
 *     null
 * @input: pointer of leaf node
 * @return: pointer of new leaf
 */
INTERNALCALL lbs_23tree_node_t* lbs_23tree_handle_null_leaf(
    lbs_23tree_node_t* leaf
    )
{
    lbs_23tree_node_t* parent;
    lbs_23tree_key_t* key[8];
    void** value[8];
    int i = 0;

    assert(IS_LEAF_NODE(leaf));
    assert(!IS_ROOT_NODE(leaf));

    if (IS_LEFT_CHILD(leaf)) {
        return leaf;
    }

    parent = GET_PARENT_NODE(leaf);
    assert(parent);
    
    key[i] = &parent->lchild->lkey;
    value[i] = &parent->lchild->lvalue;

    if (IS_TRI_NODE(parent->lchild)) {
        i++;
        key[i] = &parent->lchild->rkey;
        value[i] = &parent->lchild->rvalue;
    }

    i++;
    key[i] = &parent->lkey;
    value[i] = &parent->lvalue;
    
    if (IS_TRI_NODE(parent)) {
        i++;
        key[i] = &parent->mchild->lkey;
        value[i] = &parent->mchild->lvalue;

        if (IS_TRI_NODE(parent->mchild)) {
            i++;
            key[i] = &parent->mchild->rkey;
            value[i] = &parent->mchild->rvalue;
        }

        i++;
        key[i] = &parent->rkey;
        value[i] = &parent->rvalue;
    }

    i++;
    key[i] = &parent->rchild->lkey;
    value[i] = &parent->rchild->lvalue;

    if (IS_TRI_NODE(parent->rchild)) {
        i++;
        key[i] = &parent->rchild->rkey;
        value[i] = &parent->rchild->rvalue;
    }

    if (IS_MID_CHILD(leaf)) {
        i = 4;
    }

    for (; i > 0; i--) {
        *key[i] = *key[i-1];
        *value[i] = *value[i-1];
    }

    CLR_LKEY_VALUE(parent->lchild);
    CLR_RKEY_VALUE(parent->lchild);

    return parent->lchild;
}

INTERNALCALL
lbs_status_t lbs_23tree_fix_null_node(lbs_23tree_node_t* target_node)
{
    lbs_23tree_node_t* parent = target_node->parent;
    lbs_23tree_node_t* node = target_node;

    if (IS_LEAF_NODE(node)) {
        if (IS_ROOT_NODE(node)) {
            int* level = GET_LEVEL_POINTER(node->parent);
            *level -= 1;
            CHANGE_ROOT_NODE(null_node, node);
            free(node);
            return LBS_CODE_OK;
        }
        //node = lbs_23tree_handle_null_leaf(node);
    }

    /* This is non-left side child */
    if (!IS_LEFT_CHILD(node)) {
        if (IS_RIGHT_CHILD(node)) {
            /* This is a right child */
            if (IS_TRI_NODE(parent)) {
                lbs_23tree_node_t* mid_bro = parent->mchild;

                if (IS_TRI_NODE(mid_bro)) {
                   /*      [B , E]               [B,D]
                    *     /   |   \    ===>     /  |  \
                    *   [A] [C,D] [N]        [A]  [C]  [E]
                    *    |  | | | | |        | |  | |  | |
                    *    ^  ^ ^ ^ ^ ^        ^ ^  ^ ^  ^ ^
                    */
                    SWAP_KEY_VALUE(parent->rkey, node->lkey);   SWAP_KEY_VALUE(parent->rvalue, node->lvalue);
                    SWAP_KEY_VALUE(mid_bro->rkey, parent->rkey);SWAP_KEY_VALUE(mid_bro->rvalue, parent->rvalue);
                    if (!IS_LEAF_NODE(node)) {
                        SET_RIGHT_CHILD(node, node->lchild);
                        SET_LEFT_CHILD(node, mid_bro->rchild);
                        SET_RIGHT_CHILD(mid_bro, mid_bro->mchild);
                        CLR_MID_CHILD(mid_bro);
                    }
                } else {
                   /*      [B,D]               [B]
                    *     /  |  \    ===>     /   \
                    *   [A] [C] [N]         [A]  [C,D]
                    *    |  | | | |         | |  | | |
                    *    ^  ^ ^ ^ ^         ^ ^  ^ ^ ^
                    */
                    SWAP_KEY_VALUE(parent->rkey, mid_bro->rkey);
                    SWAP_KEY_VALUE(parent->rvalue, mid_bro->rvalue);

                    SET_RIGHT_CHILD(parent, mid_bro);
                    CLR_MID_CHILD(parent);
                    if (!IS_LEAF_NODE(node)) {
                        SET_MID_CHILD(mid_bro, mid_bro->rchild);
                        SET_RIGHT_CHILD(mid_bro, node->lchild);
                    }
                    free(node);
                }
            } else {
                /* Parent is 2-node */
                lbs_23tree_node_t* bro = parent->lchild;
                if (IS_TRI_NODE(bro)) {
                   /*      [C]               [B]
                    *     /   \    ===>     /   \
                    *  [A,B]   [N]        [A]   [C]
                    *  | | |    |         | |   | |
                    *  ^ ^ ^    ^         ^ ^   ^ ^
                    */
                    SWAP_NODE_VALUE(parent, node);
                    SWAP_KEY_VALUE(parent->lkey, bro->rkey);
                    SWAP_KEY_VALUE(parent->lvalue, bro->rvalue);
                    if (!IS_LEAF_NODE(node)) {
                        SET_RIGHT_CHILD(node, node->lchild);
                        SET_LEFT_CHILD(node, bro->rchild);
                        SET_RIGHT_CHILD(bro, bro->mchild);
                        CLR_MID_CHILD(bro);
                    }
                } else {
                   /*      [B]            [N]
                    *     /   \    ===>    |
                    *   [A]   [N]        [A,B]
                    *   | |    |         | | |
                    *   ^ ^    ^         ^ ^ ^
                    */
                    SWAP_KEY_VALUE(bro->rkey, parent->lkey);
                    SWAP_KEY_VALUE(bro->rvalue, parent->lvalue);

                    if (!IS_LEAF_NODE(node)) {
                        SET_MID_CHILD(bro, bro->rchild);
                        SET_RIGHT_CHILD(bro, node->lchild);
                    }
                    free(node);
                    if (IS_ROOT_NODE(parent)) {
                        int* level = GET_LEVEL_POINTER(parent->parent);
                        *level -= 1;
                        CHANGE_ROOT_NODE(bro, parent);
                        free(parent);
                    } else {
                        CLR_RIGHT_CHILD(parent);
                        return lbs_23tree_fix_null_node(parent);
                    }
                }
            }
        } else if (IS_MID_CHILD(node)) {
            /* This is a middle child */
            lbs_23tree_node_t* bro = parent->rchild;

            if (IS_TRI_NODE(bro)) {
               /*      [B,C]               [B,D]
                *     /  |  \    ===>     /  |  \
                *   [A] [N] [D,E]       [A] [C] [E]
                *    |   |  | | |       | | | | | |
                *    ^   ^  ^ ^ ^       ^ ^ ^ ^ ^ ^
                */
                SWAP_KEY_VALUE(parent->rkey, node->lkey); SWAP_KEY_VALUE(parent->rvalue, node->lvalue);
                SWAP_KEY_VALUE(parent->rkey, bro->lkey);  SWAP_KEY_VALUE(parent->rvalue, bro->lvalue);
                SWAP_KEY_VALUE(bro->lkey, bro->rkey);     SWAP_KEY_VALUE(bro->lvalue, bro->rvalue);

                if (!IS_LEAF_NODE(node)) {
                    SET_RIGHT_CHILD(node, bro->lchild);
                    SET_LEFT_CHILD(bro, bro->mchild);
                    CLR_MID_CHILD(bro);
                }
            } else {
               /*      [B,C]               [B]
                *     /  |  \    ===>     /   \
                *   [A] [N] [D]         [A]  [C,D]
                *    |   |  | |         | |  | | |
                *    ^   ^  ^ ^         ^ ^  ^ ^ ^
                */
                SWAP_KEY_VALUE(bro->lkey, bro->rkey); SWAP_KEY_VALUE(bro->lvalue, bro->rvalue);
                SWAP_KEY_VALUE(parent->rkey, bro->lkey); SWAP_KEY_VALUE(parent->rvalue, bro->lvalue);

                CLR_MID_CHILD(parent);
                if (!IS_LEAF_NODE(node)) {
                    SET_MID_CHILD(bro, bro->lchild);
                    SET_LEFT_CHILD(bro, node->lchild);
                }
                free(node);
            }
        }

        return LBS_CODE_OK;
    }

    /* This is a left child node. There are two main kinds of situations */
    if (IS_TRI_NODE(parent)) {
        /* Parent is 3-node */
        lbs_23tree_node_t* mid_bro = parent->mchild;

        if (IS_TRI_NODE(mid_bro)) {
            /*      [A , D]               [B,D]
             *     /   |   \    ===>     /  |  \
             *   [N] [B,C] [E]        [A]  [C]  [E]
             *    |  | | | | |        | |  | |  | |
             *    ^  ^ ^ ^ ^ ^        ^ ^  ^ ^  ^ ^
             */
            SWAP_KEY_VALUE(parent->lkey, node->lkey); SWAP_KEY_VALUE(parent->lvalue, node->lvalue);
            SWAP_KEY_VALUE(parent->lkey, mid_bro->lkey); SWAP_KEY_VALUE(parent->lvalue, mid_bro->lvalue);
            SWAP_KEY_VALUE(mid_bro->lkey, mid_bro->rkey); SWAP_KEY_VALUE(mid_bro->lvalue, mid_bro->rvalue);

            if (!IS_LEAF_NODE(node)) {
                SET_RIGHT_CHILD(node, mid_bro->lchild);
                SET_LEFT_CHILD(mid_bro, mid_bro->mchild);
                CLR_MID_CHILD(mid_bro);
            }
        } else {
            /*      [A,C]               [C]
             *     /  |  \    ===>     /   \
             *   [N] [B] [D]        [A,B]  [D]
             *    |  | | | |        | | |  | |
             *    ^  ^ ^ ^ ^        ^ ^ ^  ^ ^
             */
            SWAP_KEY_VALUE(node->lkey, parent->lkey);
            SWAP_KEY_VALUE(node->lvalue, parent->lvalue);

            SWAP_KEY_VALUE(parent->lkey, parent->rkey);
            SWAP_KEY_VALUE(parent->lvalue, parent->rvalue);

            SWAP_KEY_VALUE(node->rkey, mid_bro->lkey);
            SWAP_KEY_VALUE(node->rvalue, mid_bro->lvalue);

            if (!IS_LEAF_NODE(node)) {
                node->mchild = mid_bro->lchild;
                node->rchild = mid_bro->rchild;
                mid_bro->lchild->parent = node;
                mid_bro->rchild->parent = node;
            }
            parent->mchild = 0;

            free(mid_bro);
        }
    } else {
        /* Parent is 2-node */
        lbs_23tree_node_t* bro = parent->rchild;
        
        if (IS_TRI_NODE(bro)) {
            /*      [A]               [B]
             *     /   \    ===>     /   \
             *   [N]  [B,C]        [A]   [C]
             *    |   | | |        | |   | |
             *    ^   ^ ^ ^        ^ ^   ^ ^
             */
            SWAP_KEY_VALUE(node->lkey, parent->lkey);
            SWAP_KEY_VALUE(node->lvalue, parent->lvalue);

            SWAP_KEY_VALUE(parent->lkey, bro->lkey);
            SWAP_KEY_VALUE(parent->lvalue, bro->lvalue);

            SWAP_KEY_VALUE(bro->lkey, bro->rkey);
            SWAP_KEY_VALUE(bro->lvalue, bro->rvalue);

            if (!IS_LEAF_NODE(node)) {
                node->rchild = bro->lchild;

                bro->lchild->parent = node;
                bro->lchild = bro->mchild;
                bro->mchild = 0;
            }
        } else {
            /*      [A]           [N]
             *     /   \    ===>   |  
             *   [N]  [B]        [A,B]
             *    |   | |        | | |
             *    ^   ^ ^        ^ ^ ^
             */
            SWAP_KEY_VALUE(bro->lkey, bro->rkey);
            SWAP_KEY_VALUE(bro->lvalue, bro->rvalue);
            SWAP_KEY_VALUE(parent->lkey, bro->lkey);
            SWAP_KEY_VALUE(parent->lvalue, bro->lvalue);
            
            if (!IS_LEAF_NODE(node)) {
                bro->mchild = bro->lchild;
                bro->lchild = node->lchild;
                bro->lchild->parent = bro;
            }

            free(node);

            if (IS_ROOT_NODE(parent)) {
                int* level = GET_LEVEL_POINTER(parent->parent);
                *level -= 1;
                CHANGE_ROOT_NODE(bro, parent);
                free(parent);
            } else {
                parent->lchild = bro;
                parent->rchild = 0;
                return lbs_23tree_fix_null_node(parent);
            }
        }
    }

    return LBS_CODE_OK;
}

INTERNALCALL
lbs_status_t lbs_23tree_remove_key(
    lbs_23tree_node_t* leaf,
    lbs_23tree_key_t key
    )
{
    assert(leaf && (leaf->rkey == key || leaf->lkey == key));
    assert(IS_LEAF_NODE(leaf));

    if (IS_TRI_NODE(leaf)) {
        /* This leaf is 3-node:
         *      [x]           [x] 
         *      / \    --->   / \
         *   [a,T][y]       [a] [y]
         */
        if (leaf->lkey == key) {
            SET_LKEY_VALUE(leaf, leaf->rkey, leaf->rvalue); 
        }
        CLR_RKEY_VALUE(leaf);
        return LBS_CODE_OK;
    }

    /* This leaf is 2-node. */
    CLR_LKEY_VALUE(leaf);
    CLR_RKEY_VALUE(leaf);

    return lbs_23tree_fix_null_node(leaf);
}

INTERNALCALL
lbs_23tree_node_t* lbs_23tree_search_node_or_parent(
    lbs_23tree_node_t* starter, 
    lbs_23tree_key_t key,
    lbs_tree_search_reuslt* result
    )
{
    lbs_23tree_node_t* subtree;

    if (starter == 0 || starter->lkey == INVALID_KEY_VALUE) {
        *result = SEARCH_RES_ERROR;
        return 0;
    }

    CMP_KEY_VALUE(starter, key, *result);
    if (*result < SEARCH_RES_TARGET) {
        return starter;
    }

    if (IS_LEAF_NODE(starter)) {
        return starter;
    }

    subtree = GET_POSIBLE_SUBTREE(starter, key);
    return lbs_23tree_search_node_or_parent(subtree, key, result);
}

INTERNALCALL
lbs_23tree_node_t* lbs_23tree_get_successor_and_swap(
    lbs_23tree_node_t* node,
    lbs_23tree_key_t key
    )
{
    lbs_23tree_key_t helper_key;
    lbs_23tree_node_t* successor;
    lbs_tree_search_reuslt result;

    assert(node && key != INVALID_KEY_VALUE);

    /* There is no successor for this node becuase its a leaf */
    if (node->lchild == 0) {
        return node;
    }

    helper_key = key+1;

    successor = lbs_23tree_search_node_or_parent(node, helper_key, &result);
    if (successor == 0) {
        return 0;
    }

    if (node->rkey == key) {
        SWAP_KEY_VALUE(successor->lkey, node->rkey);
        SWAP_KEY_VALUE(successor->lvalue, node->rvalue);
    } else {
        SWAP_KEY_VALUE(successor->lkey, node->lkey);
        SWAP_KEY_VALUE(successor->lvalue, node->lvalue);
    }

    return successor;
}

INTERNALCALL
int merge_nodes(
    lbs_23tree_node_t* node, 
    lbs_23tree_node_t* parent
    )
{
    assert(node && parent);

    if (IS_TRI_NODE(node)) {
        return 1;
    }
    if (node->lkey < parent->lkey) {
        SWAP_KEY_VALUE(parent->lvalue, node->lvalue);
        SWAP_KEY_VALUE(parent->lkey, node->lkey);
    } else if (node->lkey > parent->rkey) {
        SWAP_KEY_VALUE(parent->rvalue, node->lvalue);
        SWAP_KEY_VALUE(parent->rkey, node->lkey);
    } else {
        //assert(0); // do nothing, this branch should not happen
    }

    return 0;
}

INTERNALCALL
lbs_23tree_node_t* divide_node(lbs_23tree_node_t* node)
{
    lbs_23tree_node_t* dnode;

    assert(node);

    dnode = lbs_23tree_new_node(node->rvalue, node->rkey);
    if (dnode == 0) {
        return 0;
    }

    node->rvalue = 0;
    node->rkey = INVALID_KEY_VALUE;

    if (node->mchild2) {
        SET_RELATIONS(dnode, node->mchild2, null_node, node->rchild);
        node->rchild = node->mchild;
        node->mchild = 0;
        node->mchild2 = 0;
    }

    return dnode;
}

INTERNALCALL
void insert_divided_node(
    lbs_23tree_node_t* source,
    lbs_23tree_node_t* node,
    lbs_23tree_node_t* parent)
{
    assert(node && parent && source);

    if (source == parent->rchild) {
        if (parent->mchild) {
            parent->mchild2 = source;
        } else {
            parent->mchild = source;
        }
        parent->rchild = node;
        return;
    }
    if (source == parent->mchild) {
        parent->mchild2 = node;
        return;
    }
    if (source == parent->lchild) {
        if (parent->mchild) {
            parent->mchild2 = parent->mchild;
            parent->mchild = node;
        } else {
            parent->mchild = node;
        }
    }
}

INTERNALCALL
lbs_status_t lbs_23tree_insert_into(
    lbs_23tree_node_t* node, 
    lbs_23tree_node_t* parent
    )
{
    if (node == 0 || parent == 0) {
        return LBS_CODE_INV_PARAM;
    }

    if (IS_TRI_NODE(parent)) {
        lbs_23tree_node_t* dnode;

        assert(IS_TWO_NODE(node));

        merge_nodes(node, parent);
        dnode = divide_node(parent);
        if (!dnode) {
            return LBS_CODE_NO_MEMORY;
        }

        if (IS_ROOT_NODE(parent)) {
            int* level = GET_LEVEL_POINTER(parent->parent);
            CHANGE_ROOT_NODE(node, parent);
            SET_RELATIONS(node, parent, null_node, dnode);
            *level+=1;
        } else {
            dnode->parent = parent->parent;
            insert_divided_node(parent, dnode, parent->parent);
            return lbs_23tree_insert_into(node, parent->parent);
        }
    } else {
        if (parent->lkey < node->lkey) {
            SET_RKEY_VALUE(parent, node->lkey, node->lvalue);
        } else {
            SET_RKEY_VALUE(parent, parent->lkey, parent->lvalue);
            SET_LKEY_VALUE(parent, node->lkey, node->lvalue);
        }
        free(node);
    }
    return LBS_CODE_OK;
}

INTERNALCALL
lbs_23tree_node_t* lbs_23tree_check_subtree(
    lbs_23tree_node_t* node
    )
{
    lbs_23tree_node_t* temp;

    if (node == 0) {
        assert(1);
        return (lbs_23tree_node_t*)1;
    }

    if (node->lkey == INVALID_KEY_VALUE ||
        (node->lkey == INVALID_KEY_VALUE && node->rkey == INVALID_KEY_VALUE)) {
        return node;
    }
    if (node->lkey >= node->rkey) {
        return node;
    }
    if (IS_LEAF_NODE(node)) {
        if (node->mchild != 0 || node->rchild != 0 || node->mchild2 != 0) {
            return node;
        }
        return 0;
    }

    if (!node->lchild || !node->rchild) return node;
    temp = lbs_23tree_check_subtree(node->lchild);
    if (NULL != temp) return temp;
    if (IS_TRI_NODE(node)) {
        if (node->mchild == 0) return node;
        temp = lbs_23tree_check_subtree(node->mchild);
        if (NULL != temp)
            return temp;
    }
    temp = lbs_23tree_check_subtree(node->rchild);
    if (NULL != temp) return temp;

    return 0;
}

/* Upper layer */
lbs_status_t lbs_23tree_insert(
    lbs_23tree_t* tree,
    void* value,
    lbs_23tree_key_t key
    )
{
    lbs_23tree_node_t* node;
    lbs_23tree_node_t* parent;
    lbs_tree_search_reuslt result;
    lbs_status_t rc = LBS_CODE_ALREADY_EXIST;

    if (value == 0 || tree == 0) {
        return LBS_CODE_INV_PARAM;
    }
    node = lbs_23tree_new_node(value, key);
    if (node == 0) {
        return LBS_CODE_NO_MEMORY;
    }

    /* Empty tree intialization */
    if (tree->root == 0) {
        tree->root = node;
        tree->node_count++;
        tree->level = 1;
        node->parent = (lbs_23tree_node_t*)&tree->root;
        node->isroot = 1;
        return LBS_CODE_OK;
    }

    parent = lbs_23tree_search_node_or_parent(tree->root, key, &result);
    if (parent == 0) {
        free(node);
        return LBS_CODE_INTERNAL_ERR;
    }

    if (result == SEARCH_RES_TARGET) {
        free(node);
        return 2;
    }
    if (result == SEARCH_RES_PARENT) {
        rc = lbs_23tree_insert_into(node, parent);
        if (rc == 0) {
            tree->node_count++;
        }
    }

    return rc;
}

lbs_status_t lbs_23tree_search(
    lbs_23tree_t* tree,
    lbs_23tree_key_t key,
    void** value
    )
{
    lbs_23tree_node_t* node;
    lbs_tree_search_reuslt result;

    if (tree == 0 || key == INVALID_KEY_VALUE || value == 0) {
        return LBS_CODE_INV_PARAM;
    }

    node = lbs_23tree_search_node_or_parent(tree->root, key, &result);
    if (node == 0 || result > SEARCH_RES_TARGET) {
        return LBS_CODE_NOT_FOUND;
    }

    switch (result) {
    case SEARCH_RES_LVALUE:
        *value = node->lvalue;
        break;
    case SEARCH_RES_RVALUE:
        *value = node->rvalue;
        break;
    default:
        return LBS_CODE_INTERNAL_ERR;
    }
    
    return LBS_CODE_OK;
}

lbs_status_t lbs_23tree_delete(
    lbs_23tree_t* tree,
    lbs_23tree_key_t key
    )
{
    lbs_23tree_node_t* node;
    lbs_23tree_node_t* successor;
    lbs_tree_search_reuslt result;
    lbs_status_t rc;

    if (tree == 0) {
        return LBS_CODE_EMPTY_TREE;
    }
    if (key == INVALID_KEY_VALUE) {
        return LBS_CODE_INV_KEY;
    }

    node = lbs_23tree_search_node_or_parent(tree->root, key, &result);
    if (node == 0) {
        return LBS_CODE_INTERNAL_ERR;
    }
    if (result > SEARCH_RES_TARGET) {
        return LBS_CODE_NOT_FOUND;
    }

    successor = lbs_23tree_get_successor_and_swap(node, key);
    if (successor == 0) {
        return LBS_CODE_INTERNAL_ERR;
    }

    rc = lbs_23tree_remove_key(successor, key);
    if (rc == 0) {
        tree->node_count--;
    }

    //node = lbs_23tree_check_subtree(tree->root);
    //assert(node == 0);

    return rc;
}

void lbs_23tree_release(
                        lbs_23tree_t* tree
                        )
{

}
