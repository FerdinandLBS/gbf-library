#ifndef _THP_HANDLE_H_
#define _THP_HANDLE_H_

#include "thp_rwlock.h"
#include "thp_types.h"

thp_handle_t* thp_new_handle(int need_lock);
void thp_del_handle(thp_handle_t* handle);
lbs_status_t thp_handle_rlock(thp_handle_t* handle);
lbs_status_t thp_handle_wlock(thp_handle_t* handle);
lbs_status_t thp_handle_unlock(thp_handle_t* handle);

#endif
