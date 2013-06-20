#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define __USE_GNU
#include <math.h>
#include <sched.h>
#include <sys/mman.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/unistd.h>


#ifdef KUSP
#include <kusp/sched_gsched.h>
#include <kusp/ccsm.h>
#endif /* KUSP */

#ifdef CONFIG_DSUI
#include "cpu-bound-workloops_dsui.h"
#endif	/* CONFIG_DSUI */

#include "config/test_config.h"


#define MINIMUM_WORKLOOP_COUNT 1000 /* 1 thousand loops */
#define MAXIMUM_WORKLOOP_COUNT 1000000000 /* 1 billion loops */
#define DEFAULT_WORKLOOP_COUNT 100000 /* 10 thousand loops */

#define DEFAULT_THREAD_COUNT 3

#define DEFAULT_PERIOD_COUNT 1000 

#define help_string "\
	\n\nusage %s --config=<filename> [--help]\n\n\
\t--config=\t\tthe configuration file of tasks and max workloops\n\
\t--help\t\tthis menu\n\n"

/* Routine to fetch the thread id of pthread */
#define gettid() syscall(__NR_gettid)

#ifdef KUSP
int		 grp_fd		      = -1;
int		 ccsm_fd	      = -1;

#endif	/* KUSP */


thread_info* thread_config = NULL;
int thread_count = 0;

void display_help(char **argv)
{
	printf(help_string, argv[0]);
}

static inline void per_thread_workloop(int tid, int loop_count_max){

	unsigned int loop_count;
	int totally_lame_result;

	for(loop_count = 0; loop_count < loop_count_max; loop_count++){
		/* printf("T-%d: workloop[%d]\n", tid, loop_count); */
		totally_lame_result = 2 + loop_count;
	} 

}


/* This subroutine processes the command line options */
void process_options (int argc, char *argv[])
{

	int error = 0;
	int c;


	for (;;) {
		int option_index = 0;

		static struct option long_options[] = {
			{"config",           required_argument, NULL, 'c'},
			{"help",             no_argument,       NULL, 'h'},
			{NULL, 0, NULL, 0}
		};

		/*
		 * c contains the last in the lists above corresponding to
		 * the long argument the user used.
		 */
		c = getopt_long(argc, argv, "c:", long_options, &option_index);

		if (c == -1)
			break;
		switch (c) {
			case 0:
				switch (option_index) {
					case 0:
						display_help(argv);
						exit(EXIT_SUCCESS);
						break;
				}
				break;

			case 'c':
				thread_config = get_test_config(optarg, &thread_count);
				if(thread_config == NULL || thread_count == 0)
				{
				   
					printf("parse config: failed to parse any threads from config file %s (tc is null? %s) \n", optarg, thread_config == NULL ? "yes" : "no");
				}

				break;
			case 'h':
				error = 1;
				break;
		}
	}

	if (error) {
		display_help(argv);
		exit(EXIT_FAILURE);
	}
}

void* thread_function_run(void* arg)
{
	int			tid;  /* Store the thread id */
	unsigned int            period_count;
	thread_info *targs = (thread_info*) arg;

	tid = gettid();

#ifdef KUSP
 	/* If we are supposed to identify the components of the application then
	 * create a component that represents this thread and add the component
	 * to the set that represents the pipeline.
	 */	

	printf("Creating component for T-%d\n", targs->id);
	ccsm_create_component_self(ccsm_fd, targs->name);
	printf("Adding T-%d(%d) to the set\n", targs->id, tid);
	ccsm_add_member(ccsm_fd, "socket_pipeline", targs->name);


	/* If we are supposed to use group scheduling then we need to add this
	 * thread to group scheduling.
 	 */
	if(use_gs) {
		/* If we are using ccsm then we can name the thread. */
		if(use_ccsm) {
			/* Add the thread to group scheduling by name. */
			printf("Adding T-%d(%d) to the group by name\n", 
			       targs->id, tid);

			grp_name_join_group(grp_fd, "socket_pipeline", 
					    targs->name, 0);
		} else {
			/* Add the thread to group scheduling using its pid */
			printf("adding T-%d(%d) to the group by pid\n", 
			       targs->id, tid);

			grp_pid_join_group(grp_fd, "socket_pipeline", 
					   tid, targs->name);
 		}

		/* Lower priority is better 
		 * 
		 * TODO: Warning, change of pipeline_len - id to
		 *  pipeline_len - targs->id... id is no longer
		 *  strictly linear since it comes from the
		 *  configuration file
		 */
		gsched_set_member_param_int(grp_fd, "socket_pipeline", 
					    targs->name, 
					    pipeline_len - targs->id);
 	}

 	if(use_gs) {
		gsched_set_exclusive_control(grp_fd, tid);
	}
#endif	/* KUSP */

 	for(period_count = 0; period_count < targs->intervals; period_count++)
	{
		
		per_thread_workloop(tid, targs->loop_count);
	}

	printf("T-%d(%d) Completed!\n", targs->id, tid);
	return NULL;
} 


int main(int argc, char** argv)
{
	pthread_t*		thread_ids; // pthread ids of the threads in the pipeline
	int			count;
	int			ret;
	int			i;

#ifdef CONFIG_DSUI	
	DSUI_BEGIN(&argc, &argv);
#endif	/* CONFIG_DSUI */
	process_options(argc, argv);


	if(thread_count == 0)
	{
		fprintf(stderr, "Failed to get any threads from the configuration, did you forget --config=\n");
		exit(EXIT_FAILURE);
	}

#ifdef KUSP
	/* If we are using group scheduling then create a group
	 * to contain the threads of this application.
	 */	
	if (use_gs) {
		grp_fd = grp_open();
		if (grp_fd < 0) {
			perror("grp_fd");
			return 1;
		}
		if (grp_create_group(grp_fd, "socket_pipeline", "sdf_seq")) {
			perror("create group");
			return 1;
		}

		/* This could be done after all of the threads have been
		 * added to the group.
		 */
		if (gsched_install_group(grp_fd, "socket_pipeline")) {
			perror("install group");
			return 1;
		}

	}

	/* If we are supposed to name the components of this computation
	 * then we create a named set to contain the components.
	 */
	if(use_ccsm) {
		ccsm_fd = ccsm_open();
		if (ccsm_fd < 0) {
			perror("ccsm_fd");
			return 1;
		}

		ccsm_create_set(ccsm_fd, "socket_pipeline", 0);
	} 

#endif	/* KUSP */

	/* Create storage for thread ids. */
	thread_ids = malloc(sizeof(pthread_t) * thread_count);

	printf("Starting threads...\n");

	/* Create the threads. */
	thread_info *cur = NULL;
	for(i = 0, cur = thread_config; cur; cur = cur->next)
	{
		printf("new thread: id: %d name %s intervals %d loop count %d\n",
		       cur->id, cur->name, cur->intervals, cur->loop_count);

		if(pthread_create(&thread_ids[i], NULL, &thread_function_run, 
				  (void*) thread_config))
		{
			fprintf(stderr, 
				"Failed to create thread with thread_id: %d\n", 
				cur->id);
		}
		++i;
	}

	if(i != thread_count)
	{
		fprintf(stderr, 
			"error: thread_count does not match the number of threads started (started: %d, count: %d)\n", 
			i, thread_count);
		exit(EXIT_FAILURE);
	}

	printf("Created %d threads\n", thread_count);

#ifdef USE_PTHREAD_CC	
	/* Wait for the pipeline threads to start using pthread barrier sync. */
	pthread_mutex_lock(&thread_count_lock);
	if(thread_count < DEFAULT_THREAD_COUNT) {
		pthread_cond_wait(&thread_count_control, &thread_count_lock);
	} else {
		pthread_cond_broadcast(&thread_count_control);
	}
	pthread_mutex_unlock(&thread_count_lock);
#endif	/* USE_PTHREAD_CC */


	printf("All threads started\n");

	/* Wait for threads. */
	for (i = 0; i < thread_count; ++i) {
		pthread_join(thread_ids[i], NULL);
	} 

	printf("Completed!");

#ifdef KUSP
	/* Cleanup the set we created to represent the pipeline */
	if(use_ccsm) {
		/*
		 * No error check here because HGS may destroy
		 * the set for us in grp_destroy_group.
		 */
		ccsm_destroy_set(ccsm_fd, "socket_pipeline");

		close(ccsm_fd);    
	}

#endif	/* KUSP */

	free_test_config(thread_config);
	free(thread_ids);

#ifdef CONFIG_DSUI	
	DSUI_CLEANUP();
#endif	/* CONFIG_DSUI */

	exit(EXIT_SUCCESS);
}
