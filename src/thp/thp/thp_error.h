#ifndef _THP_ERROR_H_
#define _THP_ERROR_H_

#include "thp_types.h"

void thp_set_last_error(lbs_status_t error);
lbs_status_t thp_get_last_error(void);

#endif
