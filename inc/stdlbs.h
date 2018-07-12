#ifndef _STDLBS_H_
#define _STDLBS_H_

#include <lbs.h>

#define LBS_SAFE_STRSIZE 131072

#ifdef __cplusplus
extern "C" {
#endif	

    void lbs_memcpy(void* dst, const void* src, int len);

    void lbs_memset(void* dst, char value, unsigned len);

    void lbs_strcpy(char* dst, const char* src, int max);

    lbs_bool_t lbs_strcmp(const char* s1, const char* s2, int max);

    unsigned lbs_strlen(const char* str);

    char* lbs_strleft(const char* src, unsigned len);

    char* lbs_strleft2(const char* src, char needle);

    void lbs_uppercase(char* str);

    void lbs_lowercase(char* str);
    
    void lbs_free(void*);

#ifdef __cplusplus
};
#endif


#endif