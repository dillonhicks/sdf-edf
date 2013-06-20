#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "test_config.h"
#include "xhashconf.h"


static threadspec* get_new_threadspec();

//Public:
threadspec* get_test_config(kusp_config *config, int *t_count)
{
	
	kusp_config *thread_config = NULL;
	kusp_config *kc_elem = NULL;
	kusp_attr   *kc_attr, *kc_tmp_attr;
	threadspec  *tspec_list = NULL;
	threadspec  *tspec_elem = NULL;

	*t_count = 0;
	
	if (!config) {
		printf("error: null config\n");

		return NULL;

	} else {

		HASH_FIND_STR(config->children, CONFIG_THREAD_NAME, thread_config); 
		
	}

	if (!thread_config) {
		printf("error: failed to find threads in config\n");
		return NULL;		
	}


	DL_FOREACH(thread_config, kc_elem){

		tspec_elem = get_new_threadspec();				

		HASH_ITER(hh, kc_elem->attributes, kc_attr, kc_tmp_attr) {

			if (!strcmp(kc_attr->name, "id"))
				tspec_elem->id = atoi(kc_attr->value);
			else if (!strcmp(kc_attr->name, "name"))
				 strcpy(tspec_elem->name, kc_attr->value);
			else if (!strcmp(kc_attr->name, "intervals"))
				tspec_elem->intervals = atol(kc_attr->value);
			else if (!strcmp(kc_attr->name, "loop_count"))
				tspec_elem->loop_count = atol(kc_attr->value);
			else if (!strcmp(kc_attr->name, "period_sec"))
				tspec_elem->period.tv_sec = atol(kc_attr->value);
			else if (!strcmp(kc_attr->name, "period_nsec"))
				tspec_elem->period.tv_nsec = atol(kc_attr->value);
			else if (!strcmp(kc_attr->name, "cpu"))
				 tspec_elem->cpu = atol(kc_attr->value);
			else
				printf("warning: unknown threadspec attribute %s", 
				       kc_attr->name);
		}

		(*t_count)++;
		DL_APPEND(tspec_list, tspec_elem);

	}

	return tspec_list;
}

void free_test_config(threadspec *head)
{
	threadspec *cur = NULL, *next = NULL;

	DL_FOREACH_SAFE(head, cur, next){
		DL_DELETE(head, cur);
	}
}


/**
 *
 */
void pprint_threadspec(threadspec *head){
	
	threadspec *ts;
	int count = 0;
	
	DL_FOREACH(head, ts) {

		printf("[%d] threadspec\n", count);
		printf("-----------------\n");
		printf("id:\t\t%d\n", ts->id);
		printf("name:\t\t%s\n", ts->name);
		printf("intervals:\t%d\n", ts->intervals);
		printf("loop_count:\t%d\n", ts->loop_count);
		printf("period:\t%lds %ldns\n", (unsigned long)ts->period.tv_sec, ts->period.tv_nsec);  
		printf("cpu:\t\t%d\n", ts->cpu);
		printf("has next:\t%s\n", ts->next != NULL ? "yes" : "no");		
		printf("\n");       	       
		
		count++;
	}
	
	printf("thread count: %d\n\n", count); 	
}


static threadspec* get_new_threadspec()
{
	threadspec *ti = (threadspec*)malloc(sizeof(threadspec));     
	ti->id = 0;
	strcpy(ti->name, "None");
	ti->intervals = 0;
	ti->loop_count = 0;
	ti->period.tv_sec = 0;
	ti->period.tv_nsec = 0;
	ti->cpu = 0;

	ti->next = NULL;
	ti->prev = NULL;
	return ti;
}


