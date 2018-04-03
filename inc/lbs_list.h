


#ifndef _LBS_LIST_H_
#define _LBS_LIST_H_

#include "lbs_def.h"

typedef struct _lbs_list lbs_list_t;
struct _lbs_list
{
    void* data;       /* pointer of data */
    lbs_u32_t size;   /* size of data */
    lbs_list_t* next; /* pointer of next node */
    lbs_list_t* prev; /* pointer of prev node */
};


#ifdef __cplusplus
extern "C" {
#endif

    /* [New list]
    ** Create new list node to be used.
    ** This function is failed if return value is null.
    */
    lbs_list_t* lbs_list_new(
        lbs_u8_t* data, /* pointer of data */ 
        lbs_u32_t size  /* size of data */
        );

    /* [Append a node to list]
    ** head must exist
    ** node must exist
    */
    lbs_u32_t lbs_list_append(
        lbs_list_t* head, /* pointer of the head of the list */
        lbs_list_t* node  /* pointer of the node to be appended */
        );

    /* [Find a node contain specified data]
    ** Return pointer of the node if success.
    ** Return NULL if failed
    */
    lbs_list_t* lbs_list_find(
        lbs_list_t* head, /* pointer of head */
        void* data		  /* pointer of data */
        );

    /* [Find the tail node of a list]
    ** Return pointer of a node if success.
    ** Return NULL if failed.
    */
    lbs_list_t* lbs_list_find_tail(lbs_list_t* head);

    /* [Remove a node from list] */
    lbs_u32_t lbs_list_remove(
        lbs_list_t* head, /* pointer of head node */
        void* data		  /* pointer of data */
        );

    /* [Find a node contain specified data]
    ** Return pointer of the node if success.
    ** Return NULL if failed
    */
    lbs_list_t* lbs_list_find_ex(
        lbs_list_t* head, /* pointer of head */
        void* data,	      /* pointer of data */
        int(*callback)(void* data1, void* data2) /* pointer of callback function */
        );

    /* [Destroy all the list] 
    ** Free every node in list.
    */
    lbs_u32_t lbs_list_destroy(
        lbs_list_t* head /* pointer of head node */
        );

#ifdef __cplusplus
}
#endif

#endif