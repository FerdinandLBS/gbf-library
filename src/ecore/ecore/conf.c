#include "conf.h"
#include "types.h"
#include "sys.h"

static ECORE_MODULE_LIST_NODE* module_list_node_alloc(const char* name, const char* path, unsigned mode, unsigned type)
{
    ECORE_MODULE_LIST_NODE* out;

    out = (ECORE_MODULE_LIST_NODE*)calloc(1, sizeof(ECORE_MODULE_LIST_NODE));
    if (!out)
        ecore_sys_terminate();

    ecore_strcpy(out->name, sizeof(out->name), name);
    ecore_strcpy(out->path, sizeof(out->path), path);
    out->mode = mode;
    out->type = type;

    return out;
}

static const char* conf_strcpy_end_with_char(char* dst, unsigned size, const char* src, char needle, char needle2)
{
    if (!dst || !src || !size)
        return 0;

    while (--size && (*dst = *src)) {
        if (*dst == needle || *dst == needle2) {
            *dst = 0;
            break;
        }
        dst++;
        src++;
    }

    return dst;
}

static const char* parse_config_next_item(
    const char* conf,
    char* name,
    unsigned name_size,
    char* path,
    unsigned path_size,
    unsigned* mode,
    unsigned* type
    )
{
    const char* s = conf;

    s = ecore_strstr(s, ecore_strlen(s)+1, "name=");
    if (!s)
        return 0;
    s += 5;

    conf_strcpy_end_with_char(name, name_size, s, '\n', 0x0d);

    s = ecore_strstr(s, ecore_strlen(s)+1, "path=");
    if (!s)
        return 0;
    s += 5;

    conf_strcpy_end_with_char(path, path_size, s, '\n', 0x0d);

    s = ecore_strstr(s, ecore_strlen(s)+1, "mode=");
    if (!s)
        return 0;
    s += 5;

    *mode = *s-'0'; 

    s = ecore_strstr(s, ecore_strlen(s)+1, "type=");
    if (!s)
        return 0;
    s += 5;

    *type = *s-'0';

    return s;
}

void config_list_free(ECORE_MODULE_LIST_NODE* list)
{
    ECORE_MODULE_LIST_NODE* node = list;

    while (node) {
        ECORE_MODULE_LIST_NODE* tmp = node;

        node = node->next;

        free(tmp);
    }
}

ecore_status_t config_to_list(ECORE_MODULE_LIST_NODE** list, const char* conf)
{
    ECORE_MODULE_LIST_NODE* head;
    ECORE_MODULE_LIST_NODE* node;
    ECORE_MODULE_LIST_NODE* tmp;
    char name[ECORE_MODULE_NAME_LEN];
    char path[ECORE_MODULE_PATH_LEN];
    const char* p = conf;
    unsigned mode, type;
    
    p = parse_config_next_item(p, name, sizeof(name), path, sizeof(path), &mode, &type);
    if (!p)
        return ECORE_CODE_BAD_CONFIG_FILE;
    head = module_list_node_alloc(name, path, mode, type);
    node = head;

    while (1) {
        p = parse_config_next_item(p, name, sizeof(name), path, sizeof(path), &mode, &type);
        if (!p)
            break;

        tmp = module_list_node_alloc(name, path, mode, type);
        node->next = tmp;
        tmp->prev = node;
        node = tmp;
    }

    *list = head;

    return ECORE_CODE_OK;
}
