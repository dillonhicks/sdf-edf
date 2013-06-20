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
#include <kusp/sched_hgs.h>
#include <sdf_edf.h> /* Change to <kusp/sdf_edf.h> if std install */
#include <kusp/ccsm.h>
#endif /* KUSP */

#include <edfup0_dsui.h>
#include <config/test_config.h>


/* Definitions  */

#define get_maxcpus() sysconf(_SC_NPROCESSORS_ONLN)
#define get_tid() syscall(__NR_gettid)

#define MINIMUM_WORKLOOP_COUNT 1000 /* 1 thousand loops */
#define MAXIMUM_WORKLOOP_COUNT 1000000000 /* 1 billion loops */
#define DEFAULT_WORKLOOP_COUNT 100000 /* 10 thousand loops */

#define DEFAULT_THREAD_COUNT 3
#define DEFAULT_PERIOD_COUNT 1000 

#define HGS_GROUP_NAME "edfup0"

#define TRUE 1
#define FALSE 0

#define help_string "\
	\n\nusage %s --config=<filename>  [--pprint] [--help]\n\n\
\t--config=\t\tthe configuration file of tasks and max workloops\n\
\t--pprint\t\tpretty print the confiugration after parsing\n\
\t--help\t\t\tthis menu\n\n"



/* Test Program Global Variables */

#ifdef KUSP
int		 hgs_fd		      = -1;
int		 ccsm_fd	      = -1;
#endif	/* KUSP */

#ifdef PTHREAD_CC
/* Mutex protecting our condition variable */
pthread_mutex_t	 thread_count_lock    = PTHREAD_MUTEX_INITIALIZER;	
/* Our condition variable */
pthread_cond_t	 thread_count_control = PTHREAD_COND_INITIALIZER;	

int thread_count = 0;
#endif /* PTHREAD_CC */



thread_info* thread_config = NULL;
int num_threads = 0;
int pprint_config = FALSE;



void display_help(char **argv)
{
	printf(help_string, argv[0]);
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
			{"pprint",           no_argument,       NULL, 'p'},
			{"help",             no_argument,       NULL, 'h'},
			{NULL, 0, NULL, 0}
		};

		/*
		 * c contains the last in the lists above corresponding to
		 * the long argument the user used.
		 */
		c = getopt_long(argc, argv, "c:p", long_options, &option_index);

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
			thread_config = get_test_config(optarg, &num_threads);
			if(thread_config == NULL || num_threads == 0)
			{
				
				printf("parse config: failed to parse any \
                                        threads from config file %s (tc is \
                                        null? %s) \n", 
				       optarg, 
				       thread_config == NULL ? "yes" : "no");
			}
			
			break;
		case 'p':
			pprint_config = TRUE;
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


static inline void thread_workloop(int tid, int loop_count_max){

	unsigned int loop_count;       

	DSTRM_EVENT(THREAD, WORKLOOP_BEGIN, tid);
	for(loop_count = 0; loop_count < loop_count_max; loop_count++){
		; /* Lame upcount loop */ 
	} 	
	DSTRM_EVENT(THREAD, WORKLOOP_END, tid);
}



void* thread_function_run(void* arg)
{

	cpu_set_t          cpuset;
	unsigned int       period_count;
	thread_info        *targs = (thread_info*) arg;
	int                ret;             

	DSTRM_EVENT(THREAD, BEGIN, targs->id);

	CPU_ZERO(&cpuset);
	if(targs->cpu < get_maxcpus()){
		
		CPU_SET(targs->cpu, &cpuset);
	}
	else {
		fprintf(stderr,
			"CPU_SET: Unable to set thread cpu to %d",
			targs->cpu);
		fprintf(stderr, "CPU_SET: defaulting to CPU-0");
	}
	
	/* TODO: Add the extra data of the cpu to the event */
	DSTRM_EVENT(THREAD, SET_CPU, targs->id);
	sched_setaffinity(get_tid(), sizeof(cpu_set_t), &cpuset);

#ifdef KUSP
 	/* If we are supposed to identify the components of the application then
	 * create a component that represents this thread and add the component
	 * to the set that represents the pipeline.
	 */	

	printf("ccsm_create_component: Creating component for [%d] : %s\n", 
	       targs->id, targs->name);

	if (ccsm_create_component_self(ccsm_fd, targs->name)) {
		perror("ccsm_create_component_self");
		pthread_exit(NULL);
	}

	printf("ccsm_add_member: Adding [%d] : %s to the set\n", 
	       targs->id, targs->name);
	if (ccsm_add_member(ccsm_fd, HGS_GROUP_NAME, targs->name)) {
		perror("ccsm_add_member");
		pthread_exit(NULL);
	}

	printf("sdf_edf_set_member_deadline:%s->%d\n", targs->name, targs->deadline);
	if (sdf_edf_set_member_deadline(hgs_fd, HGS_GROUP_NAME, targs->name, targs->deadline)){
		perror("sdf_edf_set_member_deadline");
		pthread_exit(NULL);
	}		

#endif	/* KUSP */

#ifdef PTHREAD_CC
	/* Block until all threads have started.  If this is the last
	 * thread to start then notify everyone that setup has
	 * completed.
	 */
	pthread_mutex_lock(&thread_count_lock);
	++thread_count;
	if(thread_count < num_threads) {
		pthread_cond_wait(&thread_count_control, &thread_count_lock);
	} else {
		pthread_cond_broadcast(&thread_count_control);		
	}
	pthread_mutex_unlock(&thread_count_lock);
	
#endif /* PTHREAD_CC */


 	for(period_count = 0; period_count < targs->intervals; period_count++)
	{	
	
#ifdef KUSP		
		ret = sdf_edf_next_instance(hgs_fd, HGS_GROUP_NAME, targs->name);
		if(ret){
			perror("sdf_edf_next_instance");
			pthread_exit(NULL);
		}
#endif /* KUSP */

		thread_workloop(targs->id, targs->loop_count);
	}

#ifdef NOT_USED
	if(hgs_leave_group(hgs_fd, HGS_GROUP_NAME, targs->name)){
		perror("hgs_leave_group");
		pthread_exit(NULL);
	}		
#endif /* NOT_USED */
	
	DSTRM_EVENT(THREAD, END, targs->id);
	printf("%s Completed!\n", targs->name);

	pthread_exit(NULL);
} 


int main(int argc, char** argv)
{
	pthread_t*	 thread_ids; 
	int		 i;

	DSUI_BEGIN(&argc, &argv);

	process_options(argc, argv);

	/* Pretty print the config if the user specified such with
	 * --pprint.
	 */
	if(pprint_config)
		pprint_thread_info(thread_config);


	if(num_threads == 0)
	{
		fprintf(stderr, "Failed to parse any thread specifications \
                        from the configuration, did you forget \
                        --config=\n");
		exit(EXIT_FAILURE);
	}

#ifdef KUSP

	
	hgs_fd = hgs_open();
	if (hgs_fd < 0) {
		perror("hgs_open");		     
		exit(hgs_fd);
	}

	ccsm_fd = ccsm_open();
	if (ccsm_fd < 0) {
		perror("ccsm_open");		     
		exit(ccsm_fd);
	}
	
	if(ccsm_create_set(ccsm_fd, HGS_GROUP_NAME, 0))
	{
		perror("ccsm_create_set");
	        exit(EXIT_FAILURE);
	}

	DSTRM_EVENT(MAIN, SET_NUM_MEMBERS, num_threads);
	if(sdf_edf_set_num_members(hgs_fd, HGS_GROUP_NAME, (unsigned int)num_threads))
	{
		perror("sdf_edf_set_num_members");
 		exit(EXIT_FAILURE);
	}

#endif	/* KUSP */

	/* Create storage for thread ids. */
	thread_ids = malloc(sizeof(pthread_t) * num_threads);

	printf("Starting threads...\n");
	
	

	/* Create the threads. */
	thread_info *cur = NULL;
	for(i = 0, cur = thread_config; cur; cur = cur->next)
	{
		printf("pthread_create(): id: %d name %s intervals %d loop count %d\n",
		       cur->id, cur->name, cur->intervals, cur->loop_count);

		if(pthread_create(&thread_ids[i], NULL, &thread_function_run, 
				  (void*) cur))
		{			
			fprintf(stderr, 
				"Failed to create thread with thread_id: %d\n", 
				cur->id);
		}
		++i;
	}

	if(i != num_threads)
	{
		fprintf(stderr, 
			"Error: num_threads does not match the \
			number of threads started (started: %d, count: %d)\n", 
			i, num_threads);
		exit(EXIT_FAILURE);
	}

#ifdef PTHREAD_CC

	/* Wait for the pipeline threads to start. */
	pthread_mutex_lock(&thread_count_lock);
	if(thread_count < num_threads) {
		pthread_cond_wait(&thread_count_control, &thread_count_lock);
	} else {
		pthread_cond_broadcast(&thread_count_control);
	}
	pthread_mutex_unlock(&thread_count_lock);

#endif /* PTHREAD_CC */
  
	printf("All threads started\n");

	/* Wait for threads to terminate. */
	for (i = 0; i < num_threads; ++i) {
		pthread_join(thread_ids[i], NULL);
	} 

	printf("Completed!\n");


	/*
	 * Cleanup Phase 
	 */

#ifdef KUSP
	/*
	 * No error check here because HGS may destroy
	 * the set for us in grp_destroy_group.
	 */
	ccsm_destroy_set(ccsm_fd, HGS_GROUP_NAME);

	close(ccsm_fd);        

#endif	/* KUSP */

	free_test_config(thread_config);
	free(thread_ids);


	DSUI_CLEANUP();

	exit(EXIT_SUCCESS);
}
