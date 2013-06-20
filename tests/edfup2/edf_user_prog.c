#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define __USE_GNU
#include <sched.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <time.h>

#ifdef HAVE_KUSP
#include <kusp/sched_hgs.h>
#include <sdf_edf.h> /* Change to <kusp/sdf_edf.h> if std install */
#include <kusp/ccsm.h>
#endif /* HAVE_KUSP */

#ifdef HAVE_GSL
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#endif	/* HAVE_GSL */

#include <edf_user_prog.h>
#include <edf_user_prog_dsui.h>


/* Test Program Global Variables */
#ifdef HAVE_KUSP
int		 Hgs_fd    = -1;
int		 Ccsm_fd   = -1;
#endif	/* HAVE_KUSP */

#ifdef HAVE_GSL
/* gnu scientific state */
const gsl_rng_type *Rng_type;
gsl_rng *Nsleep_rng;
#endif	/* HAVE_GSL */

/* User cmd line parameters */
struct user_params Params = {
	.thread_config = NULL,
	.thread_count = 0,
	.pprint = FALSE
};

/**
 * Signal handler for the RT signal that is produced when a member
 * misses its deadline.
 */
void member_missed_deadline_handler(int n, siginfo_t *info, void unused) {
	printf("Sig Handler Params:\n");
	printf("Member: %d\n", info->si_int);
	printf("N: %d\n", n);
}


/**
 * The thread workloop with nanosleeps, executed from
 * thread_run,
 */
static void thread_workloop(int tid, int loop_count_max){

	unsigned int loop_count;       
	
#ifdef HAVE_GSL
	timespec_t sleep_time = {
		.tv_sec = 0,
		.tv_nsec = gsl_ran_poisson(Nsleep_rng, (double)DEFAULT_SLEEP_NSECS)
	};
#endif	/* HAVE_GSL */

	timespec_t remaining_time;

	DSTRM_EVENT(THREAD, WORKLOOP_BEGIN, tid);


#ifdef HAVE_GSL
	DSTRM_EVENT_DATA(THREAD, NANOSLEEP, tid, sizeof(long), &sleep_time.tv_nsec, "print_long");

	for (loop_count = 0; loop_count < loop_count_max; loop_count++){
		; /* Lame upcount loop */ 
	} 

        if (nanosleep(&sleep_time, &remaining_time)) {
#else  /* HAVE_GSL */
	if (nanosleep(&default_sleep_time, &remaining_time)) {
#endif /* HAVE_GSL */
	
		fprintf(stderr,
			"[%d]nanosleep: stopped with %ld sec %ld ns remaining",
			tid,
			(unsigned long)remaining_time.tv_sec,
			remaining_time.tv_nsec);
		
	}


	DSTRM_EVENT(THREAD, WORKLOOP_END, tid);
}


/**
 * The per thread function code. 
 * 
 * - Sets the thread's CPU
 * 
 * - Create's a CCSM component for the thread 
 * 
 * - Sets the deadline of the member
 * 
 * - Executes the workloop with the desired number of instances and iterations.
 */
void* thread_run(void* arg)
{

	cpu_set_t          cpuset;
	unsigned int       period_count;
	threadspec         *targs = (threadspec*) arg;
	int                ret;             

	DSTRM_EVENT(THREAD, BEGIN, targs->id);

	CPU_ZERO(&cpuset);
	if (targs->cpu < get_maxcpus()) {
		
		CPU_SET(targs->cpu, &cpuset);
	} else {
		fprintf(stderr,
			"CPU_SET: Unable to set thread cpu to %d",
			targs->cpu);
		fprintf(stderr, "CPU_SET: defaulting to CPU-0");
	}
	
	DSTRM_EVENT_DATA(THREAD, SET_CPU, targs->id, sizeof(cpu_set_t), &cpuset, "print_int");
	sched_setaffinity(get_tid(), sizeof(cpu_set_t), &cpuset);

#ifdef HAVE_KUSP

 	/**
	 * If we are supposed to identify the components of the
	 * application then create a component that represents this
	 * thread and add the component to the set that represents the
	 * pipeline.
	 */	
	printf("ccsm_create_component: Creating component for [%d] : %s\n", 
	       targs->id, targs->name);

	if (ccsm_create_component_self(Ccsm_fd, targs->name)) {
		perror("ccsm_create_component_self");
		pthread_exit(NULL);
	}

	printf("ccsm_add_member: Adding [%d] : %s to the set\n", 
	       targs->id, targs->name);

#ifdef NOT_USED

	/**
	 * This is not needed with the configuration file method
	 * because when CCSM sets get bound to a task the the group
	 * will now see that the set is now bound to a task and then
	 * it is added to a group via hgs_type_group.bind
	 */
	if (ccsm_add_member(Ccsm_fd, HGS_GROUP_NAME, targs->name)) {
		perror("ccsm_add_member");
		pthread_exit(NULL);
	}

#endif	/* NOT_USED */
	printf("sdf_edf_set_member_period:%s-> %ld:%ld\n", targs->name, 
	       (unsigned long)targs->period.tv_sec, targs->period.tv_nsec);

	if (sdf_edf_set_member_period(Hgs_fd, HGS_GROUP_NAME, 
					targs->name, &targs->period)) {
		perror("sdf_edf_set_member_period");
		pthread_exit(NULL);
	}		

#endif	/* HAVE_KUSP */


 	for (period_count = 0; period_count < targs->intervals; period_count++) {	
	
#ifdef HAVE_KUSP		
		ret = sdf_edf_next_instance(Hgs_fd, HGS_GROUP_NAME, targs->name);
		if (ret) {
			perror("sdf_edf_next_instance");
			pthread_exit(NULL);
		}
#endif /* HAVE_KUSP */

		thread_workloop(targs->id, targs->loop_count);
		/* printf("%s - %d\n", targs->name, period_count); */
	}
	
	DSTRM_EVENT(THREAD, END, targs->id);
	printf("%s Completed!\n", targs->name);

	pthread_exit(NULL);
} 



/**
 *  This subroutine processes the command line options 
 */
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

		/**
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
				printf(help_string, argv[0]);
				exit(EXIT_SUCCESS);
				break;
			}
			break;
			
		case 'c':
			/* Parse the whole config using the standard config parser */
			Params.config = kusp_parse_xml_config(optarg);

			/* Get the thread section for ease of use */
			Params.thread_config = get_test_config(Params.config, &Params.thread_count);

			if (!Params.thread_config || !Params.thread_count){
				
				printf("parse config: failed to parse any"
                                        "threads from config file %s (tc is"
                                        "null? %s) \n", 
				       optarg, 
				       Params.thread_config == NULL ? "yes" : "no");
			}
			
			break;
		case 'p':
			Params.pprint = TRUE;
			break;

		case 'h':
			error = 1;
			break;

		}
	}
	
	if (error) {
		printf(help_string, argv[0]);
		exit(EXIT_FAILURE);
	}
}




int main(int argc, char** argv)
{
	pthread_t*	 thread_ids; 
	int		 i;
	struct           sigaction sig;

	DSUI_BEGIN(&argc, &argv);

	process_options(argc, argv);

	/**
	 * Setup the signal handler for SIG_TEST SA_SIGINFO -> we want
 	 * the signal handler function with 3 arguments
 	 */
	sig.sa_sigaction = member_missed_deadline_handler;
	sig.sa_flags = SA_SIGINFO;
	sigaction(SIG_MISSED_DEADLINE, &sig, NULL);

	/**
	 * Pretty print the config if the user specified such with
	 * --pprint.
	 */
	if (Params.pprint)
		pprint_threadspec(Params.thread_config);


	if (Params.thread_count == 0) {
		fprintf(stderr,"Failed to parse any thread specifications"
                        "from the configuration, did you forget --config=\n");
		exit(EXIT_FAILURE);
	}


#ifdef HAVE_GSL
	/* gnu scientific stuff */
	gsl_rng_env_setup();
	Rng_type = gsl_rng_default;
	Nsleep_rng = gsl_rng_alloc(Rng_type);
#endif	/* HAVE_GSL */



#ifdef HAVE_KUSP


	/*
	 * Obtaining the IOCTL file descriptors for the calls to HGS
	 * and CCSM  
	 */
	Hgs_fd = hgs_open();
	if (Hgs_fd < 0) {
		perror("hgs_open");		     
		exit(Hgs_fd);
	}

	Ccsm_fd = ccsm_open();
	if (Ccsm_fd < 0) {
		perror("ccsm_open");		     
		exit(Ccsm_fd);
	}

#ifdef NOT_USED	
	/**
	 * This is not necissary when using a configuration file
	 * Create the set for the group
         */
	if (ccsm_create_set(Ccsm_fd, HGS_GROUP_NAME, 0)) {
		perror("ccsm_create_set");
	        exit(EXIT_FAILURE);
	}
#endif /* NOT_USED */

	/**
	 * Set the number of members that the group expects for
	 * barrier sync
	 */
	DSTRM_EVENT(MAIN, SET_NUM_MEMBERS, Params.thread_count);
	if (sdf_edf_set_num_members(Hgs_fd, HGS_GROUP_NAME, 
				    (unsigned int)Params.thread_count)) {

		perror("sdf_edf_set_num_members");
 		exit(EXIT_FAILURE);
	}

#endif	/* HAVE_KUSP */

	/* Create storage for thread ids. */
	thread_ids = malloc(sizeof(pthread_t) * Params.thread_count);

	printf("Starting threads...\n");
	       
	/* Create the threads. */
	i = 0;
	threadspec *targs = Params.thread_config;
	while(targs) {		
	
		printf("pthread_create(): id: %d name %s intervals %d loop count %d\n",
		       targs->id, targs->name, targs->intervals, targs->loop_count);
		
		/*
		 * Create the new pthread that will execute
		 * thread_run(), store its id (handle) within the
		 * array of ids, and send it the approriate pointer to
		 * the arguments. 
		 */
		if (pthread_create(&thread_ids[i], NULL, thread_run, (void*) targs)) {			
			fprintf(stderr, 
				"Failed to create thread with thread_id: %d\n", 
				targs->id);
		}
		
		/* 
		 * Get the next threadspec and increment the creation
		 * counter
		 */
		targs = targs->next;
		i++;
	}
		
	if (i != Params.thread_count) {
		fprintf(stderr, 
			"Error: Params.thread_count does not match the \
			number of threads started (started: %d, count: %d)\n", 
			i, Params.thread_count);
		exit(EXIT_FAILURE);
	}
  
	printf("All threads created.\n");

	/* Wait for threads to terminate. */
	for (i = 0; i < Params.thread_count; i++) {
		pthread_join(thread_ids[i], NULL);
	} 

	printf("Completed!\n");

	/*
	 * Cleanup Phase: Closing file descriptors, de-allocating
	 * memory and exiting.
	 */

#ifdef HAVE_KUSP
	/* This is bogus and is not needed for the configuration file method. 
	 *
	 * No error check here because HGS may destroy
	 * the set for us in grp_destroy_group.
	 */
	ccsm_destroy_set(Ccsm_fd, HGS_GROUP_NAME);
	
	/* Always close your file descriptors, always*/
	close(Ccsm_fd);        
	close(Hgs_fd);

#endif	/* HAVE_KUSP */


#ifdef HAVE_GSL
	gsl_rng_free(Nsleep_rng);
#endif	/* HAVE_GSL */

	free_test_config(Params.thread_config);
	free(thread_ids);

	DSUI_CLEANUP();

	exit(EXIT_SUCCESS);
}
