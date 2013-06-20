#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#define MAX_THREAD_NAME_LENGTH 25

typedef struct _thread_info
{
	unsigned int id;
	char name[MAX_THREAD_NAME_LENGTH];

	unsigned int intervals;
	unsigned int loop_count;
	unsigned int deadline;
	unsigned int cpu;

	struct _thread_info *next;	

} thread_info;

thread_info* get_test_config(char *filename, int *count);
void free_test_config(thread_info *head);
void pprint_thread_info(thread_info *head);

#endif
