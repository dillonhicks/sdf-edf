#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

typedef struct _thread_info
{
	unsigned int id;
	char name[10];

	unsigned int intervals;
	unsigned int loop_count;

	struct _thread_info *next;
} thread_info;

thread_info* get_test_config(char *filename, int *count);
void free_test_config(thread_info *head);

#endif
