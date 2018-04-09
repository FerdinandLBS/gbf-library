#ifndef _THP_LOG_H_
#define _THP_LOG_H_

#define THP_LOG_TO_SCREEN 0
#define THP_LOG_TO_FILE   1
#define THP_LOG_TO_MEMORY 2

int thp_log_init(int mode, char* filename);
int thp_log_push(char* format, ...);

#endif
