#include <string.h>
#include <sched_hgs.h>
#include <linux/hgs_sdf_edf.h>

/**
 * Set a deadline for a member (in nanoseconds).
 * 
 * @fd Group scheduling file descriptor.
 * @grp Group name that contains the member to search for.
 * @mem The name of the member of which to set the deadline.
 * @deadline The deadline for the member in nanoseconds.
 */
int sdf_edf_set_member_deadline(int fd, const char *grp, const char *mem, 
			 unsigned long deadline)
{
	struct sdf_edf_mp param;

	memset(&param, 0, sizeof(param));
	param.cmd = SDF_EDF_MP_CMD_DEADLINE;
	param.deadline = deadline;

	return hgs_set_member_parameters(fd, grp, mem, &param, sizeof(param));
} 


int sdf_edf_next_instance(int fd, const char *grp, const char *mem){

	struct sdf_edf_mp param;

	memset(&param, 0, sizeof(param));
	param.cmd = SDF_EDF_MP_CMD_STATUS;
	param.status = SDF_EDF_MEM_FINISHED;

	return hgs_set_member_parameters(fd, grp, mem, &param, sizeof(param));
	
}				

int sdf_edf_set_num_members(int fd, const char *grp, unsigned int num_mems){

	struct sdf_edf_gp param;

	memset(&param, 0, sizeof(param));
	param.cmd = SDF_EDF_GP_CMD_NUM_MEMS;
	param.num_mems = num_mems;

	return hgs_set_group_parameters(fd, grp, &param, sizeof(param));

}
