#include <stdlbs.h>
#include <stdlib.h>

void lbs_memcpy(void* dst, const void* src, int len)
{
    unsigned char* p1 = (unsigned char*)dst, *p2 = (unsigned char*)src;

    while (len--) {
        p1[len] = p2[len];
    }
}

void lbs_memset(void* dst, char value, unsigned len)
{
    char* p = dst;

    while (len--) {
        *p++ = value;
    }
}

void lbs_strcpy(char* dst, const char* src, int max)
{
    char* p1 = dst;
    const char* p2 = src;

    while (max--) {
        *p1 = *p2;
        if (*p2 == 0) break;
        p1++; p2++;
    }
}

lbs_bool_t lbs_strcmp(const char* s1, const char* s2, int max)
{
	lbs_bool_t ret = LBS_TRUE;
    unsigned real_max = LBS_MAX_U32;

    if (max > 0) {
        real_max = (unsigned)max;
    }

	while(!(ret = *(unsigned char *)s1 - *(unsigned char *)s2) && *s2 && *s1 && real_max)
		++s1, ++s2, real_max--;

	if (ret < 0)
		ret = LBS_FALSE;
	else if ( ret > 0 )
		ret = LBS_FALSE;

	return ret;
}

unsigned lbs_strlen(const char* str)
{
    unsigned out = 0;
	const char* p = str;

    while (*p++);

    return (p-str-1);
}

char* lbs_strleft(const char* src, unsigned len)
{
    char* res = 0;
    unsigned i = 0;

    if (len == 0)
        return 0;

    res = malloc(len+1);
    if (!res)
        return 0;

    while (i < len && src[i] != 0)
        res[i] = src[i++];

    res[len] = 0;
    return res;
}

char* lbs_strleft2(const char* src, char needle)
{
    char* res = 0;
    int i = 0;

#ifdef LBS_PERFORMANCE
    int buffer_size = 0;
    res = malloc(32);
    while (src[i] != needle) {
        if (i >= buffer_size-1) {
            buffer_size += 10;
            res = realloc(res, buffer_size);
        }
        res[i] = src[i++];
    }
#else
    do {
        if (!src[i] || src[i] == needle)
            break;
    } while (i++ < 65536);

    res = malloc(i+1);
    if (res == 0)
        return 0;

    lbs_memcpy(res, src, i);
    res[i] = 0;
#endif

    return res;
}

void lbs_free(void* p)
{
    free(p);
}

void lbs_uppercase(char* str)
{
    unsigned i = 0;

    if (!str) return;

    while (str[i] && i < LBS_SAFE_STRSIZE) {
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] -= 32;

        i++;
    }
}

void lbs_lowercase(char* str)
{
    unsigned i = 0;

    if (!str) return;

    while (str[i] && i < LBS_SAFE_STRSIZE) {
        if (str[i] >= 'A' && str[i] <= 'Z')
            str[i] += 32;

        i++;
    }
}
