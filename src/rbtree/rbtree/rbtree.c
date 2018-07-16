#include <stdlib.h>
#include <rbtree.h>

#define IS_LEAF_NODE(_node_) ((_node_->lnode == 0) || _node_->lnode != 0 &&)
#define KEY_EXIST 1
#define KEY_INSERTABLE 2

#define RBTREE_COLOR_BLACK 0
#define RBTREE_COLOR_RED   1

#define GET_NODE_COLOR(_node_) (_node_->status&0x01)
#define GET_CHILD_COLOR(_node_) ((_node_->status&0x02)>>1)

#define HAS_RED_LINK(_node_) (_node_->status&0x02)
#define IS_ROOT(_node_) (_node_->status&0x04)
#define IS_LEFT(_node_) (_node_->status&0x08)
#define IS_RED(_node_) (_node_->status&0x01)
#define IS_BLACK(_node_) (!IS_RED(_node_))
#define IS_ALONE(_node_) (IS_BLACK(_node) && !HAS_RED_LINK(_node_))

#define SET_ROOT(_node_) (_node_->status|=0x04)
#define SET_NODE(_node_) (_node_->status&=0xFB)
#define SET_BLACK(_node_) (_node_->status&=0xFE)
#define SET_RED(_node_) (_node_->status|=0x01)
#define SET_BLACK_LINK(_node_) (_node_->status&=0xFD)
#define SET_RED_LINK(_node_) (_node_->status|=0x02)
#define SET_LEFT_NODE(_node_) (_node_->status|=0x08)
#define SET_RIGHT_NODE(_node_) (_node_->status&=0xF7)

#define SWAP_VALUE(A,B) do{(long)A=(long)A^(long)B;(long)B=(long)A^(long)B;(long)A=(long)A^(long)B;}while(0)
#define SWAP_NODE_DATA(_node1_, _node2_) do{SWAP_VALUE(_node1_->key, _node2_->key); SWAP_VALUE(_node1_->data, _node2_->data);}while(0)

#define FIX_CHILDREN_RELATOIN(_node_) do{if (_node_->lnode) _node_->lnode->parent = _node_; if (_node_->rnode) _node_->rnode = _node_; } while (0)

__inline static void build_relation(rbtree_node_t* parent, rbtree_node_t* left_node, rbtree_node_t* right_node)
{
    parent->lnode = left_node;
    parent->rnode = right_node;
    left_node->parent = parent;
    right_node->parent = parent;
}

__inline static void build_relation_check(rbtree_node_t* parent, rbtree_node_t* left_node, rbtree_node_t* right_node)
{
    parent->lnode = left_node;
    parent->rnode = right_node;
    if (left_node)
        left_node->parent = parent;
    if (right_node)
        right_node->parent = parent;
}

__inline static void build_relation_left(rbtree_node_t* parent, rbtree_node_t* node)
{
    parent->lnode = node;
    node->parent = parent;
}

__inline static void build_relation_right(rbtree_node_t* parent, rbtree_node_t* node)
{
    parent->rnode = node;
    node->parent = parent;
}

__inline static void build_relation_left_check(rbtree_node_t* parent, rbtree_node_t* node)
{
    parent->lnode = node;
    if (node)
        node->parent = parent;
}

__inline static void build_relation_right_check(rbtree_node_t* parent, rbtree_node_t* node)
{
    parent->rnode = node;
    if (node)
        node->parent = parent;
}

__inline static void change_root(rbtree_t* tree, rbtree_node_t* old_root, rbtree_node_t* new_root, int depth)
{
    tree->depth += depth;
    tree->root = new_root;
    if (new_root) {
        new_root->parent = (void*)tree;
        SET_ROOT(new_root);
    }
    SET_NODE(old_root);
}

static int check_integrity(rbtree_node_t* root)
{
    if (!root)
        return 0;
    if (root->lnode) {
        if (root->key <= root->lnode->key)
            return 1;

        if (IS_RED(root) && IS_RED(root->lnode))
            return 1;

        if (HAS_RED_LINK(root) && IS_BLACK(root->lnode))
            return 1;

        if (check_integrity(root->lnode))
            return 1;
    } else {
        if (HAS_RED_LINK(root))
            return 1;
    }
    if (root->rnode) {
        if (!root->lnode)
            return 1;

        if (root->key >= root->rnode->key)
            return 1;

        if (check_integrity(root->rnode))
            return 1;
    }
    
    return 0;
}

static rbtree_node_t* get_key_location(rbtree_node_t* root, int key, int* status)
{
    if (root->key == key) {
        *status = KEY_EXIST;
        return root;
    }

    if (root->key > key) {
        if (root->lnode) {
            return get_key_location(root->lnode, key, status);
        }
    } else {
        if (root->rnode) {
            return get_key_location(root->rnode, key, status);
        }
    }

    *status = KEY_INSERTABLE;
    return root;
}

int rbt_new_tree(rbtree_t** tree, unsigned init_pool_size, unsigned mode)
{
    rbtree_t* out = 0;
    rbtree_node_t* pool;
    int rc = RBT_CODE_INV_PARAM;

    (void)mode;

    if (!tree)
        goto bail;

    out = (rbtree_t*)calloc(1, sizeof(rbtree_t));
    if (out == 0)
        return RBT_CODE_NO_MEMORY;

    if (init_pool_size) {
        out->pool_size = init_pool_size;
        while (init_pool_size--) {
            pool = (rbtree_node_t*)calloc(1, sizeof(rbtree_node_t));
            if (pool == 0) {
                rc = RBT_CODE_NO_MEMORY;
                goto bail;
            }
            if (out->pool == 0) {
                out->pool = pool;
            } else {
                pool->parent = out->pool;
                out->pool = pool;
            }
        }
    }

    out->mode = mode;
    *tree = out;
    rc = RBT_CODE_OK;

bail:
    if (out && rc) {
        while (out->pool) {
            pool = out->pool;
            out->pool = out->pool->parent;
            free(pool);
        }
        free(out);
    }
    return rc;
}

static rbtree_node_t* pool_pop_node(rbtree_t* tree)
{
    if (tree->pool) {
        rbtree_node_t* node;
        node = tree->pool;
        tree->pool = tree->pool->parent;
        return node;
    }

    return (rbtree_node_t*)calloc(1, sizeof(rbtree_node_t));
}

static int rbt_insert_root(rbtree_t* tree, int key, void* data)
{
    rbtree_node_t* root;

    root = pool_pop_node(tree);
    if (!root)
        return RBT_CODE_NO_MEMORY;

    SET_ROOT(root);
    SET_BLACK(root);
    root->data = data;
    root->key = key;
    root->parent = (void*)tree;

    tree->count = 1;
    tree->depth = 1;
    tree->root = root;

    return RBT_CODE_OK;
}

rbtree_node_t* rbt_search(rbtree_t* tree, int key)
{
    rbtree_node_t* node;

    if (!(tree->root))
        return 0;

    node = tree->root;
    while (1) {
        if (node->key == key) {
            break;    
        } else if (node->key > key) {
            if (node->lnode)
                node = node->lnode;
            else
                return 0;
        } else {
            if (node->rnode)
                node = node->rnode;
            else
                return 0;
        }
    }

    return node;
}

rbtree_node_t* rbt_first(rbtree_node_t* root)
{
    rbtree_node_t* node = root;

    if (!root)
        return 0;

    while (1) {
        if (node->lnode == 0)
            return node;

        node = node->lnode;
    }
}

rbtree_node_t* rbt_last(rbtree_node_t* root)
{
    rbtree_node_t* node = root;

    if (!root)
        return 0;

    while (1) {
        if (node->rnode == 0)
            return node;

        node = node->rnode;
    }
}

rbtree_node_t* rbt_next(rbtree_node_t* current)
{
    rbtree_node_t* node = current;
    rbtree_node_t* parent;

    if (!current)
        return 0;

    /* find next unpicked parent node */
    if (node->rnode == 0) {
        while (1) {
            parent = node->parent;
            if (IS_ROOT(node))
                return 0;

            if (node == parent->rnode)
                node = parent;
            else
                return parent;
        }
    }

    node = node->rnode;
    while (1) {
        if (node->lnode)
            node = node->lnode;
        else
            return node;
    }
}

rbtree_node_t* rbt_prev(rbtree_node_t* current)
{
    rbtree_node_t* node = current;
    rbtree_node_t* parent;

    if (!current)
        return 0;

    /* find next unpicked parent node */
    if (node->lnode == 0) {
        while (1) {
            parent = node->parent;
            if (IS_ROOT(node))
                return 0;

            if (node == parent->lnode)
                node = parent;
            else
                return parent;
        }
    }

    node = node->lnode;
    while (1) {
        if (node->rnode)
            node = node->rnode;
        else
            return node;
    }
}

int rbt_insert(rbtree_t* tree, int key, void* data, lbs_bool_t is_update, void** old_data)
{
    rbtree_node_t* node;
    rbtree_node_t* parent;
    rbtree_node_t* grandpa;

    if (is_update == LBS_TRUE && old_data == 0)
        return LBS_CODE_INV_PARAM;

    /* empty tree */
    if (tree->root == 0) {
        rbt_insert_root(tree, key, data);
        if ((tree->mode&RBT_MODE_INTEGRITY) && check_integrity(tree->root))
            return RBT_CODE_CORRUPT;

        return 0;
    }

    /* find the location to insert the node */
    node = tree->root;
    while (1) {
        if (node->key == key) {
            if (is_update) {
                *old_data = node->data;
                node->data = data;
                return 0;
            } else {
                return RBT_CODE_ALREADY_EXSIT;
            }
        } else if (node->key > key) {
            if (node->lnode) {
                node = node->lnode;
                continue;
            }
        } else {
            if (node->rnode) {
                node = node->rnode;
                continue;
            }
        }
        break;
    }

    /* prepare initailization data for loop */
    parent = node;
    node = pool_pop_node(tree);
    if (node == 0)
        return RBT_CODE_NO_MEMORY;
    node->data = data;
    node->key = key;

    /* process insert */
    while (1) {
        grandpa = parent->parent;
        if (IS_RED(parent) || HAS_RED_LINK(parent)) {
            /* parent is a red node or has red link */
            rbtree_node_t* parent2; // parent2 is always the black one

            if (IS_RED(parent)) {
                parent2 = parent->parent; 
                grandpa = parent2->parent;
            } else {
                parent2 = parent;
                parent = parent->lnode;
            }
            
            SET_BLACK(parent);
            SET_BLACK_LINK(parent2);

            if (parent->key > node->key) {
               /*     g           p
                *   // \         / \
                *   P   u  ==>  n   g
                *  / \             / \
                * n   b           b   u
                */
                build_relation_left_check(parent2, parent->rnode);
                build_relation(parent, node, parent2);

                node = parent;
                parent = grandpa;
            } else if (parent2->key < node->key) {
               /*     p           p
                *   // \         / \
                *   b   n  ==>  b   n
                */
                build_relation_right(parent2, node);

                if (IS_ROOT(parent2)) {
                    tree->depth++;
                    break;
                }
                node = parent2;
                parent = grandpa;
            } else {
               /*       g            n
                *     // \         /   \
                *     P   u  ==>  p     g
                *    / \         / \   / \
                *   b   n       b   l r   u
                *      / \
                *     l   r
                */

                build_relation_right_check(parent, node->lnode);
                build_relation_left_check(parent2, node->rnode);
                build_relation(node, parent, parent2);

                parent = grandpa;
            }

            if (IS_ROOT(parent2)) {
                change_root(tree, parent2, node, 1);
                break;
            }
        } else {
            /* parent is a black node without red link */

            if (parent->key > node->key) {
                /*     p         P
                 *    / \  ==> // \
                 *   n   b     N   b
                 */
                build_relation_left(parent, node);
                SET_RED(node); 
                SET_RED_LINK(parent);
                break;
            } else {
                /*    p           n
                 *   / \   ==>  // \
                 *  b   n       P   r
                 *     / \     / \
                 *    l   r   b   l
                 */
                if (IS_ROOT(parent)) {
                    change_root(tree, parent, node, 0);
                } else {
                    if (grandpa->lnode == parent) 
                        grandpa->lnode = node;
                    else 
                        grandpa->rnode = node;
                }

                build_relation_right_check(parent, node->lnode);
                build_relation_left(node, parent);
                node->parent = grandpa;

                SET_RED(parent); 
                SET_RED_LINK(node);

                break;
            }
        }
    }

    tree->count++;

    
    if ((tree->mode&RBT_MODE_INTEGRITY) && check_integrity(tree->root))
        return RBT_CODE_CORRUPT;

    return 0;
}

int rbt_remove(rbtree_t* tree, int key, void** data)
{
    rbtree_node_t* node;
    rbtree_node_t* op;
    rbtree_node_t* parent;
    rbtree_node_t* bro;
    rbtree_node_t* nephew;

    /* does key exist? */
    node = rbt_search(tree, key);
    if (node == 0)
        return RBT_CODE_NOT_FOUND;
    
    /* ouput data */
    if (data)
        *data = node->data;

    /* get successor and swap data. make sucessor the first operated node */
    if (node->rnode) {
        op = rbt_first(node->rnode);
        SWAP_NODE_DATA(node, op);
    } else {
        op = node;
    }

    if (HAS_RED_LINK(op)) {
        SWAP_NODE_DATA(op, op->lnode);
        SET_BLACK_LINK(op);
        op = op->lnode;
        op->parent->lnode = 0;

        goto bail;
    }


    /*
     * Legend: 
     *   p : parent
     *   op: operated node
     *   b : brother
     *   n : nephew
     *   g : grandparent
     *   gs: grandson
     *   w,x,y,z: sub tree
     *   /,\: black link
     *   //: red link
     *   lower case: black node
     *   UPPER CASE: red node
     */
    while (1) {
        if (IS_ROOT(op)) {
            change_root(tree, op, op->lnode, -1);
            break;
        }

        parent = op->parent;

        /* get brother node */
        if (op == parent->lnode) {
            bro = parent->rnode;
        } else {
            bro = parent->lnode;
        }

        if (IS_RED(op)) {
            /*    p        p
             *  // \      / \
             * OP   b => w   b
             * |    
             * w   (w and b would be nil)
             */
            build_relation_left_check(parent, op->lnode);
            SET_BLACK_LINK(parent);
            if (parent->lnode)
                SET_BLACK(parent->lnode);

            break;
        }

        if (IS_RED(bro)) {
            nephew = bro->rnode;
            if (HAS_RED_LINK(nephew)) {
               /*     P            n
                *   // \        //   \ 
                *   B   op  =>  B     p  
                *  / \  |      / \    |\
                * ^   n z     ^  gs   y z
                *    //\         / \
                *   GS  y       w   x
                *   /\
                *  w  x
                */
                rbtree_node_t* gs = nephew->lnode;
                SWAP_NODE_DATA(parent, op);
                SWAP_NODE_DATA(nephew, parent);
                build_relation_right_check(op, op->lnode);
                build_relation_left_check(op, nephew->rnode);
                build_relation_right(bro, gs);
                SET_BLACK(gs);
                op = nephew;
                break;
            } else {
                /*      p             b
                 *    // \           / \
                 *    B   op  =>    ^   p
                 *   /\    |          // \
                 *  ^  n   z          n   z
                 *     /\            /\
                 *    x  y          x  y
                 */
                SWAP_NODE_DATA(parent, op);
                SWAP_NODE_DATA(parent, bro);
                build_relation_right_check(op, op->lnode);
                build_relation_left(parent, bro->lnode);
                build_relation_left(op, nephew);
                SET_BLACK_LINK(parent);
                SET_RED(nephew);
                SET_RED_LINK(op);
                op = bro;
                break;
            }
        } else if (HAS_RED_LINK(bro)) {
            nephew = bro->lnode;
            if (op == parent->lnode) {
               /*     p            n
                *    / \          / \
                *  op   b   =>   p   b
                *   |  //\      /\   /\
                *   x  N  z    x  y w  z
                *     /\
                *    y  w
                */
                SWAP_NODE_DATA(op, parent);
                SWAP_NODE_DATA(parent, nephew);
                build_relation_right_check(op, nephew->lnode);
                build_relation_left_check(bro, nephew->rnode);
                SET_BLACK_LINK(bro);
                op = nephew;
                break;
            } else {
                /*       p              b
                 *      / \            / \
                 *     b   op         n   p
                 *   //\    |  =>    /\   /\
                 *   N  w   z       x  y w  z 
                 *  /\
                 * x  y
                 */
                SWAP_NODE_DATA(bro, parent);
                SWAP_NODE_DATA(op, bro);
                build_relation_left(parent, nephew);
                build_relation_left_check(op, bro->rnode);
                SET_BLACK(nephew);
                op = bro;
                break;
            }
        } else {
            /* parent stands alone: we always link the only child on left side */
            if (op == parent->lnode) {
                /*    p           op
                 *   / \          |
                 *  op  b         b     
                 *  |  / \  =>  // \
                 *  x y   z     P   z
                 *             / \
                 *            x   y
                 */
                 SWAP_NODE_DATA(parent, op);
                 build_relation_right_check(op, bro->lnode);
                 build_relation_left(bro, op);
                 build_relation_left(parent, bro);
                 SET_RED(op);
                 SET_RED_LINK(bro);
                 op = parent;
                 continue;
            } else {
                /*     p         op
                 *    / \        |
                 *   b  op =>    p
                 *  /\   |     // \
                 * x  y  z     B   z  
                 *            /\
                 *           x  y
                 */
                SWAP_NODE_DATA(op, parent);
                build_relation_left(op, bro);
                build_relation_left(parent, op);
                SET_RED(bro);
                SET_RED_LINK(op);
                op = parent;
                continue;
            }
        }
    }

bail:
    free(op);
    tree->count--;
    
    if ((tree->mode&RBT_MODE_INTEGRITY) && check_integrity(tree->root))
        return RBT_CODE_CORRUPT;

    return 0;
}


static void delete_sub_tree(rbtree_node_t* root)
{
    if (root->lnode)
        delete_sub_tree(root->lnode);

    if (root->rnode)
        delete_sub_tree(root->rnode);

    free(root);
}

// TODO: we should provide callback function to handle data destruction
void rbt_delete_tree(rbtree_t* tree)
{
    if (!tree)
        return;

    if (tree->root)
        delete_sub_tree(tree->root);

    free(tree);

    return;
}
