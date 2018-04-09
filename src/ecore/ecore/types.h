#ifndef _ECORE_TYPES_H_
#define _ECORE_TYPES_H_

#include <lbs_thread.h>

/*
 * ecore running mode
 */
#define ECORE_RUNNING_MODE_MONO     1
#define ECORE_RUNNING_MODE_PARALLEL 2

/*
 * module types
 */
#define ECORE_MODULE_TYPE_MASTER 1
#define ECORE_MODULE_TYPE_SLAVE  2
#define ECORE_MODULE_TYPE_SPRITE 3

/*
 * module running mode
 */
#define ECORE_MODULE_MODE_MONO     1
#define ECORE_MODULE_MODE_PARALLEL 2

/*
 * module string length limitation
 */
#define ECORE_MODULE_NAME_LEN 260
#define ECORE_MODULE_PATH_LEN 260

/*
 * call convention
 */
#define ECORE_CALLCONV _stdcall

/*
 * use this value for index if you want to send message to all modules
 */
#define ECORE_BROADCAST_MSG -1

/*
 * message types
 */
#define ECORE_MSG_SHUTDOWN 0    // force module shut down. no effect to master
#define ECORE_MSG_RESET    1    // ask module to restart
#define ECORE_MSG_GET_ATTR 2    // get attributes of module
#define ECORE_MSG_USER     5000 // user customized message value plus this one

typedef int (ECORE_CALLCONV *fn_find_module)(char* name);
typedef int (ECORE_CALLCONV *fn_post_message)(int index, int message, void* data);
typedef int (ECORE_CALLCONV *fn_send_message)(int index, int message, void* data);

typedef void (ECORE_CALLCONV *fn_module_onload)(int index, fn_find_module find_module, fn_post_message post_message, fn_send_message send_message);
typedef void (ECORE_CALLCONV *fn_module_onunload)(void);
typedef void (ECORE_CALLCONV *fn_module_onmessage)(unsigned message, void* data);
typedef void (ECORE_CALLCONV *fn_module_onstart)(void);

typedef struct __ecore_module_st_ {
    // unique id for tree
    unsigned id;

    // reference to ECORE_MODULE_TYPE_XXX
    unsigned type;

    // reference to ECORE_MODULE_MODE_XXX
    unsigned running_mode;
    lbs_thread_t thread;

    // the name of the module file
    char name[ECORE_MODULE_NAME_LEN];

    // the path of the module file. end with "/"
    char path[ECORE_MODULE_PATH_LEN];
    
    // the entry point of the module
    void* entry;

    // export interfaces
    fn_module_onload   onload;
    fn_module_onunload onunload;
    fn_module_onmessage  onmessage;
    fn_module_onstart  onstart;
}ECORE_MODULE;

typedef struct __ecore_config_module_list_node_st_ {
    char name[ECORE_MODULE_NAME_LEN];
    char path[ECORE_MODULE_PATH_LEN];
    unsigned mode;
    unsigned type;

    struct __ecore_config_module_list_node_st_* next;
    struct __ecore_config_module_list_node_st_* prev;
}ECORE_MODULE_LIST_NODE;

typedef int ecore_status_t;

#define ECORE_CODE_OK 0
#define ECORE_CODE_INV_PARAM 1
#define ECORE_CODE_FAIL_TO_LOAD_LIBRARY 2
#define ECORE_CODE_FAIL_TO_INSERT_MOD 3
#define ECORE_CODE_MULTIPLE_MASTERS 4
#define ECORE_CODE_ERROR_TYPE_MODE 5
#define ECORE_CODE_BAD_MODULE_ONLOAD 6
#define ECORE_CODE_BAD_MODULE_ONSTART 7
#define ECORE_CODE_BAD_MODULE_ONMSG 8
#define ECORE_CODE_BAD_MODULE_ONUNLOAD 9
#define ECORE_CODE_BAD_CONFIG_FILE 10
#define ECORE_CODE_NO_MEMORY 11
#define ECORE_CODE_NO_MASTER_FOUND 12
#define ECORE_CODE_NOT_FOUND 13

#endif