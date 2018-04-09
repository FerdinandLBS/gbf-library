#ifndef _ECORE_SYS_H_
#define _ECORE_SYS_H_

#ifdef WIN32
#include <Windows.h>
#endif

/********************************************************************
 * STRING OPERATIONS
 */

/* string compare */
char ecore_strcmp(const char* s1, const char* s2, unsigned size);

/* string copy */
char* ecore_strcpy(char* dst, unsigned size, const char* src);

/* string cat */
char* ecore_strcat(char* dst, unsigned size, const char* str);

/* find sub string */
const char* ecore_strstr(const char* str, unsigned size, const char* substr);

/* get string length. ignore the tail '0' */
unsigned ecore_strlen(const char* str);

/********************************************************************
 * SYSTEM FUNCTIONS
 */

/* load library */
void* ecore_load_library(char* name);

/* unload a library */
void ecore_unload_library(void* lib);

/* get function address */
void* ecore_get_address(void* lib, const char* name);

/* terminate the system */
void ecore_sys_terminate();


#endif