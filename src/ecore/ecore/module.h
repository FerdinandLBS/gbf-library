#ifndef _ECORE_MODULE_H_
#define _ECORE_MODULE_H_

#include "types.h"


void init_module();

void done_module();

ecore_status_t ecore_load_module_dummy(
    const char* name,
    const char* path,
    unsigned type,
    unsigned mode
    );

ecore_status_t ecore_load_modules(ECORE_MODULE_LIST_NODE* list);

ecore_status_t ecore_start_modules();

ECORE_MODULE* get_specified_module(int index);

ECORE_MODULE* get_first_module();

ECORE_MODULE* get_next_module();

#endif