#ifndef SDF_EDF_H
#define SDF_EDF_H

#define SDF_EDF_NAME "sdf_edf"
#define SDF_EDF_DEFAULT_DEADLINE 1000000 /* 1 ms */


/**
 * @SDF_EDF_MEM_SETUP After the member is inserted within the sdf_edf
 *     group and needs to get to finish setup.
 *
 * @SDF_EDF_MEM_RELEASED The member has been realeased for the next
 *     exeuction instance for the current epoch.
 *
 * @SDF_EDF_MEM_FINISHED The member has completed execution within the
 *     current epoch and needs to wait until the end of epoch to be
 *     released to start the next execution instance. The first time
 *     this status flag is set it is also used for barrier sync.
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
	SDF_EDF_MEM_RELEASED,
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
		unsigned long deadline;
		enum sdf_edf_mem_status status;
 	};
};


/**
 * @SDF_EDF_GROUP_SETUP Waiting on all members to join.
 *
 * @SDF_EDF_GROUP_OK All members have arrived, start normal scheduling.
 * 
 * @SDF_EDF_GROUP_CLEANUP Members are begining to leave the group,
 *     should follow SDF_EDF_GROUP_OK
 *
 * @SDF_EDF_GROUP_NORESTRICT Members can leave and join at will.
 */
enum sdf_edf_group_status {
	SDF_EDF_GROUP_INIT = 0,
	SDF_EDF_GROUP_RUNNING,
	SDF_EDF_GROUP_NORESTRICT,
};


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
 
