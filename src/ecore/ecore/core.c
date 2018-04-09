#include "conf.h"
#include "event.h"
#include "module.h"
#include "types.h"
#include "sys.h"

#include <rbtree.h>
#include <lbs_rwlock.h>

static ecore_status_t ecore_init(unsigned mode)
{
    init_module();
    init_event();

    return 0;
}

static char* get_conf_content(const char* path)
{
    char* out = 0;
    HANDLE fd;
    int size_low, size_high, bytes, res;

    fd = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (fd == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    size_low = GetFileSize(fd, &size_high);
    if (size_low == INVALID_FILE_SIZE && size_high == 0)
        goto bail;
    if (size_high != 0)
        goto bail;
    
    out = (char*)malloc(size_low);
    if (out == 0)
        goto bail;

    res = ReadFile(fd, out, size_low, &bytes, 0);
    if (res == 0 || size_low != bytes) {
        goto bail;
    }

    CloseHandle(fd);

    return out;
bail:
    CloseHandle(fd);
    if (out)
        free(out);
    return 0;
}

ecore_status_t ecore_start(unsigned mode, const char* conf)
{
    ECORE_MODULE_LIST_NODE* list = 0;
    ecore_status_t rc = 0;
    char* content;

    ecore_init(mode);

    content = get_conf_content(conf);
    if (content == 0)
        return ECORE_CODE_BAD_CONFIG_FILE;

    rc = config_to_list(&list, content);
    if (rc)
        goto bail;

    rc = ecore_load_modules(list);
    if (rc)
        goto bail;

    rc = ecore_start_modules();

bail:
    free(content);
    config_list_free(list);

    return rc;
}

