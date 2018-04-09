#include "module.h"
#include "sys.h"
#include "event.h"
#include <lbs_rwlock.h>
#include <rbtree.h>
#include <lbs_thread.h>
#include <stdio.h>

lbs_lock_t* __module_lock = 0;
rbtree_t* __module_tree = 0;
unsigned __module_index = 1;
int __master_index = -1;
int __is_master_loaded = 0;
int __current_index = 0;
lbs_thread_t __master_thread_handle = 0;

static void module_lock()
{
    if (lbs_rwlock_wlock(__module_lock))
        ecore_sys_terminate();
}

static void module_unlock()
{
    if (lbs_rwlock_unlock(__module_lock))
        ecore_sys_terminate();
}

/* Master module worker thread */
int LBS_THREAD_CALL master_thread(void* param)
{
    ECORE_MODULE* mod;
    rbtree_node_t* node;

    module_lock();

    node = rbt_search(__module_tree, __master_index);
    mod = node->data;

    module_unlock();

	if (mod->onstart)
		mod->onstart();

    return 0;
}

int LBS_THREAD_CALL module_thread(void* param)
{
    ECORE_MODULE* mode = param;

    if (mode->onstart)
        mode->onstart();

    return 0;
}

void init_module()
{
    int status;

    status = rbt_new_tree(&__module_tree, 0, 0);
    if (__module_tree == 0 && status != 0)
        ecore_sys_terminate();

    if (lbs_rwlock_init(&__module_lock))
        ecore_sys_terminate();

    if (lbs_create_thread(0, master_thread, LBS_THREAD_CREATE_SUSPENDED, &__master_thread_handle))
        ecore_sys_terminate();
}

static ecore_status_t insert_module_to_tree(ECORE_MODULE* p)
{
    ecore_status_t rc = 0;

    module_lock();

    if (p->type == ECORE_MODULE_TYPE_MASTER && __is_master_loaded) {
        rc = ECORE_CODE_MULTIPLE_MASTERS;
        goto bail;
    }

    p->id = __module_index++;
    if (rbt_insert(__module_tree, p->id, p, 0))
        rc = ECORE_CODE_FAIL_TO_INSERT_MOD;

bail:
    module_unlock();

    return rc;
}

static ecore_status_t check_module_status(ECORE_MODULE* p)
{
    if (p->type == ECORE_MODULE_TYPE_SLAVE && p->running_mode != ECORE_MODULE_MODE_PARALLEL)
        return ECORE_CODE_ERROR_TYPE_MODE;

    if (p->onload == 0)
        return ECORE_CODE_BAD_MODULE_ONLOAD;
    if (p->onmessage == 0)
        return ECORE_CODE_BAD_MODULE_ONMSG;
    if (p->onstart == 0)
        return ECORE_CODE_BAD_MODULE_ONSTART;
    if (p->onunload == 0)
        return ECORE_CODE_BAD_MODULE_ONUNLOAD;

    module_lock();
    if (p->type == ECORE_MODULE_TYPE_MASTER && __is_master_loaded) {
        module_unlock();
        return ECORE_CODE_MULTIPLE_MASTERS;
    }
    module_unlock();

    return 0;
}

static void prepare_module_names(ECORE_MODULE* p, const char* name, const char* path)
{
    ecore_strcpy(p->name, sizeof(p->name), name);

    if (path)
        ecore_strcpy(p->path, sizeof(p->path), path);
}

static ecore_status_t prepare_module_entries(ECORE_MODULE* p)
{
    char fullname[ECORE_MODULE_PATH_LEN+ECORE_MODULE_NAME_LEN];

    if (p->path[0]) {
        ecore_strcpy(fullname, sizeof(fullname), p->path);
        ecore_strcat(fullname, sizeof(fullname), p->name);
    } else {
        ecore_strcpy(fullname, sizeof(fullname), p->name);
    }

    p->entry = ecore_load_library(fullname);
    if (p->entry == 0) {
        printf(fullname);
        return ECORE_CODE_FAIL_TO_LOAD_LIBRARY;
    }

    p->onload    = ecore_get_address(p->entry, "ecore_onload");
    p->onstart   = ecore_get_address(p->entry, "ecore_onstart");
    p->onmessage = ecore_get_address(p->entry, "ecore_onmessage");
    p->onunload  = ecore_get_address(p->entry, "ecore_onunload");

    return 0;
}

static ECORE_MODULE* ecore_module_alloc()
{
    ECORE_MODULE* out;

    out = (ECORE_MODULE*)calloc(1, sizeof(ECORE_MODULE));
    if (!out)
        ecore_sys_terminate();

    return out;
}

void ecore_module_free(ECORE_MODULE* p)
{
    if (!p)
        return;

    if (p->entry) {
        ecore_unload_library(p->entry);
    }

    free(p);
}

void done_module()
{
    lbs_rwlock_destroy(&__module_lock);
}

ecore_status_t ecore_load_module_dummy(
    const char* name,
    const char* path,
    unsigned type,
    unsigned mode
    )
{
    ECORE_MODULE* module;
    ecore_status_t rc = 0;

    if (name == 0)
        return ECORE_CODE_INV_PARAM;

    module = ecore_module_alloc();

    prepare_module_names(module, name, path);
    
    rc = prepare_module_entries(module);
    if (rc)
        goto bail;
    
    module->type = type;
    module->running_mode = mode;

    rc = check_module_status(module);
	if (rc)
		goto bail;

    rc = insert_module_to_tree(module);
    if (rc)
        goto bail;

bail:
    if (rc) {
        ecore_module_free(module);
    }

    return rc;
}

/* load all modules in the input list into global list */
ecore_status_t ecore_load_modules(ECORE_MODULE_LIST_NODE* list)
{
    ECORE_MODULE_LIST_NODE* node = list;
    ecore_status_t rc;

    while (node) {
        rc = ecore_load_module_dummy(node->name, node->path, node->type, node->mode);
        if (rc)
            return rc;

        node = node->next;
    }

    return 0;
}

static int ECORE_CALLCONV post_message_to(int index, int message, void* data)
{
    ECORE_MESSAGE* msg;

    msg = (ECORE_MESSAGE*)malloc(sizeof(ECORE_MESSAGE));
    if (msg == 0)
        return ECORE_CODE_NO_MEMORY;

    msg->msg = message;
    msg->data = data;
    msg->module_id = index;

    return post_event(msg);
}

static int ECORE_CALLCONV send_message_to(int index, int message, void* data)
{
    ECORE_MODULE* mod;
    rbtree_node_t* node;

    module_lock();
	node = rbt_search(__module_tree, index);

	if (node == 0) {
		module_unlock();
		return ECORE_CODE_NOT_FOUND;
	}

	mod = node->data;
	if (mod->onmessage)
		mod->onmessage(message, data);
	module_unlock();

	return 0;
}

static int ECORE_CALLCONV get_module_index(char* module_name)
{
    ECORE_MODULE* mod;    
    rbtree_node_t* node;
	int index;

    module_lock();
    node = rbt_first(__module_tree->root);
    
    while (node) {
        mod = node->data;
		if (!ecore_strcmp(mod->name, module_name, sizeof(mod->name))) {
			index = node->key;
			module_unlock();

            return index;
		}

        node = rbt_next(node);
    }
    module_unlock();

    return -1;
}

static ecore_status_t start_module_onload(ECORE_MODULE* mod, int index)
{
    switch (mod->type) {
    case ECORE_MODULE_TYPE_MASTER:
        __master_index = index;
    case ECORE_MODULE_TYPE_SLAVE:
    case ECORE_MODULE_TYPE_SPRITE:
        if (mod->onload)
            mod->onload(index, get_module_index, post_message_to, send_message_to);

        break;
    default:
        return ECORE_CODE_ERROR_TYPE_MODE;
    }

    return ECORE_CODE_OK;
}

ECORE_MODULE* get_specified_module(int index)
{
    ECORE_MODULE* mod = 0;
    rbtree_node_t* node;

    module_lock();

    node = rbt_search(__module_tree, index);
    if (node)
        mod = node->data;

    module_unlock();
    
    return mod;
}

ECORE_MODULE* get_first_module()
{
    ECORE_MODULE* mod;
    rbtree_node_t* node;

    module_lock();

    node = rbt_first(__module_tree->root);
    mod = node->data;
    __current_index = node->key;

    module_unlock();
    
    return mod;
}

ECORE_MODULE* get_next_module()
{
    ECORE_MODULE* mod;
    rbtree_node_t* node = 0;
    rbtree_node_t* last;

    module_lock();

    last = rbt_last(__module_tree->root);
    if (__current_index >= last->key) {
        module_unlock();
        return 0;
    }

    __current_index++;
    while (!node) node = rbt_search(__module_tree, __current_index++);
    mod = node->data;
    __current_index = node->key;

    module_unlock();
    
    return mod;
}

/* start all modules in the global list */
ecore_status_t ecore_start_modules()
{
    ecore_status_t rc = ECORE_CODE_OK;
    rbtree_node_t* node;
    ECORE_MODULE* mod;

    module_lock();
    node = rbt_first(__module_tree->root);
    module_unlock();
    
    while (node) {
        rc = start_module_onload(node->data, node->key);
        if (rc && !__is_master_loaded) {
            break;
        }
        mod = node->data;
        if (mod->type == ECORE_MODULE_TYPE_MASTER) {
            lbs_resume_thread(__master_thread_handle);
            /* log error here */
        } else if (mod->running_mode == ECORE_RUNNING_MODE_PARALLEL) {
            lbs_create_thread(mod, module_thread, 0, &mod->thread);
            /* log error here */
        }
        node = rbt_next(node);
    }

    return rc;
}

