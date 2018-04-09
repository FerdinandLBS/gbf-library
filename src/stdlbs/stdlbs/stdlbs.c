#include <stdlbs.h>

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

int lbs_strcmp(const char* s1, const char* s2, int max)
{
	int ret = 0 ;

	while(!(ret = *(unsigned char *)s1 - *(unsigned char *)s2) && *s2 && *s1)
		++s1, ++s2;

	if (ret < 0)
		ret = -1 ;
	else if ( ret > 0 )
		ret = 1 ;

	return ret;
}

unsigned lbs_strlen(const char* str)
{
    unsigned out = 0;
	const char* p = str;

    while (*p++);

    return (p-str-1);
}
