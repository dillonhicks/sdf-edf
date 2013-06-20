#ifndef SDF_EDF_H
#define SDF_EDF_H

#define SDF_EDF_NAME "sdf_edf"

extern int sdf_edf_set_member_deadline(int fd, const char *grp, const char *mem,
				       unsigned long deadline);

extern int sdf_edf_next_instance(int fd, const char *grp, const char *mem);

extern int sdf_edf_set_num_members(int fd, const char *grp, unsigned int num_mems);

#endif
