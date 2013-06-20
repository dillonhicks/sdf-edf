#ifndef EDFUP1_H
#define EDFUP1_H

#include <config/test_config.h>
#include <xhashconf/xhashconf.h>

/* Definitions  */

#define get_maxcpus() sysconf(_SC_NPROCESSORS_ONLN)
#define get_tid() syscall(__NR_gettid)

#define MINIMUM_WORKLOOP_COUNT    1000 /* 1 thousand loops */
#define MAXIMUM_WORKLOOP_COUNT    1000000000 /* 1 billion loops */
#define DEFAULT_WORKLOOP_COUNT    100000 /* 10 thousand loops */

#define DEFAULT_THREAD_COUNT     3
#define DEFAULT_PERIOD_COUNT  1000 

#define HGS_GROUP_NAME "edfup1"

#define DEFAULT_SLEEP_SECS   0 	/* 0 seconds */
#define DEFAULT_SLEEP_NSECS 100 /* 100 ns */

#define TRUE 1
#define FALSE 0

#define help_string "\
	\n\nusage %s --config=<filename>  [--pprint] [--help]\n\n\
\t--config=\t\tthe configuration file of tasks and max workloops\n\
\t--pprint\t\tpretty print the confiugration after parsing\n\
\t--help\t\t\tthis menu\n\n"

typedef struct timespec timespec_t ;


/* Structs */

timespec_t default_sleep_time = {
	.tv_sec = DEFAULT_SLEEP_SECS,
	.tv_nsec = DEFAULT_SLEEP_NSECS
};


struct user_params {
	kusp_config  *config;
	threadspec   *thread_config;
        int          thread_count;	
	int          pprint;	

};


#endif	/* EDFUP1_H */
