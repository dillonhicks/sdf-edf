#ifndef SDF_EDF_H
#define SDF_EDF_H

#define SDF_EDF_NAME "sdf_edf"
struct timespec SDF_EDF_DEFAULT_DEADLINE = {
	.tv_sec = 0UL,		
	.tv_nsec = 1000000      
};


/**
 * @SDF_EDF_MEM_SETUP After the member is inserted within the sdf_edf
 *     group and needs to get to finish setup.
 *
 * @SDF_EDF_MEM_READY The member has been realeased for the next
 *     exeuction instance for the current epoch.
 *
 * @SDF_EDF_MEM_FINISHED The member has completed execution within the
 *     current epoch and needs to wait until the end of epoch to be
 *     released to start the next execution instance. The first time
 *     this status flag is set (SETUP->FINISHED) it will also
 *     increment the group's __ready_count. 
 * 
 * @SDF_EDF_MEM_RUNNING Similar to ready. The member has been released
 *     for the execution in the current epoch and has been chosen to
 *     run at least once.
 *
 * @SDF_EDF_MEM_MISSED_DEADLINE The member has missed its previous
 *     deadline. [ future use ]
 * 
 * @SDF_EDF_MEM_CLEANUP Member removed from the group and pending
 *     memory release/free.
 * 
 */
enum sdf_edf_mem_status{
	SDF_EDF_MEM_SETUP = 0,
	SDF_EDF_MEM_READY,
	SDF_EDF_MEM_RUNNING,
	SDF_EDF_MEM_FINISHED,       
	SDF_EDF_MEM_CLEANUP,
	
};


/* Member Parameters switch to inform the union in sdf_edf_mp */
enum sdf_edf_mp_cmd {
	SDF_EDF_MP_CMD_DEADLINE,
	SDF_EDF_MP_CMD_STATUS,
};

/** 
 * sdf edf mem params struct -- Allows users to set one
 * parameter at a time through the API.
 */
struct sdf_edf_mp {
	enum sdf_edf_mp_cmd cmd;
	union {	      
		struct timespec deadline;
		enum sdf_edf_mem_status status;
 	};
};


/**
 * @SDF_EDF_GROUP_SETUP Waiting on all members to join and set their
 *     deadlines at the sdf_edf_next_instance() barrier.
 *
 * @SDF_EDF_GROUP_RUNNING All members have arrived at the
 *     sdf_edf_next_instance() barrier and will now be scheduled
 *     according to A) their deadline and B) their status (only
 *     SDF_EDF_MEM_READY or SDF_EDF_MEM_RUNNING) tasks may be chosen.
 *
 */
enum sdf_edf_group_status {
	SDF_EDF_GROUP_SETUP = 0,
	SDF_EDF_GROUP_RUNNING,
};


/* Used as a switch for the corresponding group param in the
 * sdf_edf_gp struct for get/set group params
 */
enum sdf_edf_gp_cmd {
	SDF_EDF_GP_CMD_NUM_MEMS,
	SDF_EDF_GP_CMD_MEM_COUNT,
};


struct sdf_edf_gp {
	enum sdf_edf_gp_cmd cmd;
	union {	      
		unsigned int num_mems;
		unsigned int mem_count;
 	};
	
};



#endif /* SDF_EDF_H */
 
