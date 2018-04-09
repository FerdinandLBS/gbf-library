#ifndef _ECORE_CONF_H_
#define _ECORE_CONF_H_

#include "types.h"

ecore_status_t config_to_list(ECORE_MODULE_LIST_NODE** list, const char* conf);

void config_list_free(ECORE_MODULE_LIST_NODE* list);


#endif