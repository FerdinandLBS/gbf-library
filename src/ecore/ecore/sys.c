#include "sys.h"

char ecore_strcmp(const char* s1, const char* s2, unsigned size)
{
    char ret;

    while (--size && *s1 && *s2) {
        ret = *s1 - *s2;
        if (ret != 0)
            return -1;

        s1++; s2++;
    }

    return 0;
}

char* ecore_strcpy(char* dst, unsigned size, const char* src)
{
    if (!dst || !src || !size)
        return 0;

    while (--size && (*dst++ = *src++)) ;

    if (!size) *dst = 0;

    return dst;
}

char* ecore_strcat(char* dst, unsigned size, const char* str)
{
    if (!dst || !str || !size)
        return 0;

    while (size && *dst) {
        dst++;
        size--;
    }

    while (--size && (*dst++ = *str++)) ;

    if (!size) *dst = 0;

    return dst;
}

const char* ecore_strstr(const char* str, unsigned size, const char* substr)
{
    const char* l, *s;
    unsigned len;

    while (*str && --size) {
        l = str; s = substr; len = size;
        while (*l == *s && --len) {
            l++; s++;
            if (*s == 0)
                return str;
        }
        str++;
    }

    return 0;
}

unsigned ecore_strlen(const char* str)
{
    unsigned len = 0;
    while (*(str++)) len++;

    return len;
}

void* ecore_load_library(char* name)
{
    void* entry;
#ifdef WIN32
    entry = LoadLibraryA(name);
#else
#endif

    return entry;
}

void ecore_unload_library(void* lib)
{
#ifdef WIN32
    CloseHandle(lib);
#else
    void(lib);
#endif
}

void* ecore_get_address(void* lib, const char* name)
{
    void* addr = 0;

#ifdef WIN32
    if (lib == 0)
        return 0;

    addr = GetProcAddress(lib, name);
#else
#endif

    return addr;
}

void ecore_sys_terminate()
{
    exit(0);
}
