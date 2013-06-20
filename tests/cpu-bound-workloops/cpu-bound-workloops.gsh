# cpu_bound_workloops.gsh
#
# Test heirarchy for sdf_edf
# 
#
# Iteration 0: Just a single thread underneath the deadline with a
#   very large deadline (100ms/0.1s/10000000ns).


#
# Installation information is currently for reference 
# for expansion that is planned for the future. 
# Such as multiple hierarchies within a file, exectuable based
# hierarchy attachment points, etc.
#
<gsh-installation>
local-root = "cpu_bound_workloops"
attachment-point = "gsched_top_seq_group"


#
# GROUPS are defined as:
# 
# <group-name> = {
#          sdf = "<sdf-name>"
#	   attributes = [<attr0, 
#	   	      	<...>]
#	   per_group_data = { <datum_name> = <value> }
#	   members = [  <member-0>,
#	   	        <member-2>,
#			 ... ,
#			<member-n>
#		     ]
#	   member_name = "<member-name>"
# 	   comment = "I'm a group that does _____"
# }
#
# All fields within the group specification are required, 
# but attributes, per_group_data, and members are allowed
# empty dictionary and list values.
#

<groups>
#
# Create socket_pipeline group with 7 Thread members.
#
socket_pipeline = {
	sdf = "sdf_edf"
	attributes = [managed]
	per_group_data = {}
	members = [
				thread-0,
        ]
	member_name = "cpu_bound_workloop_mem"
	comment = ""			  
}

# Creates seven Threads that use simple-thread as a specfication
# (template).
#  <desired-thread-name> = <thread-specification-name>
#
<threads>
thread-0 = "thread-0"

<thread-specification>
thread0 = {
	attributes = [exclusive]
	per_member_data = {
		deadline = 10000000
	}
	comment = "Create member with a deadline of .1 second (1x10^7 ns).
}


