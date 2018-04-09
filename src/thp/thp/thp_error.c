#include "thp_error.h"

lbs_status_t thp_errno = 0;
thp_lock_t* lock = 0;
int inited = 0;

static int thp_error_init()
{
    if (inited != 0)
        return 0;

    if (lock == 0)
        if (thp_rwlock_init(&lock))
            return 1;

    thp_errno = 0;
    inited = 1;

    return 0;
}

void thp_set_last_error(lbs_status_t error)
{
    if (0 != thp_error_init())
        return;

    if (thp_rwlock_wlock(lock))
        return;

    thp_errno = error;

    thp_rwlock_unlock(lock);
}

lbs_status_t thp_get_last_error(void)
{
    lbs_status_t error = 0;

    if (0 != thp_error_init())
        return error;

    if (thp_rwlock_rlock(lock))
        return error;

    error = thp_errno;

    thp_rwlock_unlock(lock);

    return error;
}
