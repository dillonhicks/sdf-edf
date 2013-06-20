#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#define MAX_THREAD_NAME_LENGTH 25
#define CONFIG_THREAD_NAME "thread"

#include "xhashconf.h"

typedef struct _threadspec {
	unsigned int id;
	char name[MAX_THREAD_NAME_LENGTH];

	unsigned int intervals;
	unsigned int loop_count;
	struct timespec period;
	unsigned int cpu;

	struct _threadspec *next;	
	struct _threadspec *prev;

} threadspec;

threadspec* get_test_config(kusp_config *config, int *t_count);
void free_test_config(threadspec *head);
void pprint_threadspec(threadspec *head);

#endif	/* TEST_CONFIG_H */
