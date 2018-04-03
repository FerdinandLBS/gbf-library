#ifndef _STDLBS_H_
#define _STDLBS_H_

#ifdef __cplusplus
extern "C" {
#endif	

    void lbs_memcpy(void* dst, const void* src, int len);
    void lbs_memset(void* dst, char value, unsigned len);

    void lbs_strcpy(char* dst, const char* src, int max);
    int lbs_strcmp(const char* s1, const char* s2, int max);
    unsigned lbs_strlen(const char* str);

#ifdef __cplusplus
};
#endif


#endif