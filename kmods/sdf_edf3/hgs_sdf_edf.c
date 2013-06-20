/**
 * 
 * Earliest Deadline First SDF
 */
#include <linux/list.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/kusp/hgs.h> 
#include <linux/module.h> 
#include <asm/siginfo.h>	/* siginfo */
#include <linux/rcupdate.h>	/* rcu_read_lock */
#include <linux/kusp/dski.h>
/* change to <linux/hgs_sdf_edf.h> for std install*/

#include "../../include/linux/hgs_sdf_edf.h" 



/**
 * cpu local information about a member
 *
 * @edf_member Backpointer to member data
 * @sorted_ent Entry on cpu list
 */
struct edf_member_cpu
{
	struct edf_member *edf_member;
	struct list_head sorted_ent;
};



/**
 * Data to store on a per member basis. The member data is allocated
 * automatically by group scheduling. The SDF informs group scheduling
 * about what size its member data should be.
 *
 * @member Often, at scheduling time, it can be useful to access a
 *         group scheduling member structure from its per member data
 *         if the scheduling code is walking a list of member data.
 *         This pointer is NOT mandatory.
 * 
 * @mem_ent The list_head that is the entry for the member within the
 *          edf_group_data's grouplist.
 *
 * @period 
 *
 * @phase
 * 
 * @cpu cpu information for the member. groups use every cpu, tasks use
 *      only one cpu at a time
 */
struct edf_member 
{
	struct gsched_member    *member;
	struct list_head         mem_ent; 

	struct timespec          deadline;
	struct timespec          period;

	enum sdf_edf_mem_phase   phase;
	struct hrtimer           timer;

	struct edf_member_cpu    cpu[NR_CPUS];	
};


/**
 * Information maintained by a group for each cpu
 *
 * @sorted_members A list of members 
 */
struct edf_cpu
{
	struct list_head sorted_members;
};



/**
 * EDF per group data
 * 
 * @num_mems The number of members expected to be in this group. When
 *      num_mems > 0, this value is used as a barrier synchronization
 *      which will wait until the number of members within the group
 *      is equal to num_mems before allowing them to execute their
 *      work, otherwise (num_mems = 0) no such synchronization will
 *      happen.
 * 
 * @__mem_count Internal counter that tracks the number of members in
 *      the group (see num_mems).
 *
 * @__ready_count interal counter that tracks the nunber of members
 *     that have reached the first call of
 *     sdf_edf_next_instance(). When __ready_count and __mem_count
 *     both equal num_mems the group is switched from GROUP_SETUP to
 *     GROUP_RUNNING indicating that normal deadline based scheduling
 *     will occur.
 *
 */
struct edf_group 
{
	struct gsched_group* group;

	enum sdf_edf_group_phase phase;
	unsigned int num_mems; 
	unsigned int __mem_count;
	unsigned int __ready_count;

};


/* Forward declaration */
static void __sdf_edf_set_member_phase(struct edf_group *gd, 
				       struct edf_member *md, int next_phase);


/** 
 * TODO: Change "data" param name to reflect that it is a pointer to
 * the hrtimer struct and that we use container_of to get access to
 * the data in the member_data.
 *
 */
static enum hrtimer_restart 
sdf_edf_member_deadline_timer_callback(struct hrtimer *mem_timer) {
	
	unsigned long       flags;
	struct edf_group    *gd;
	struct edf_member   *md;
	struct gsched_group *group; 
	
	DSTRM_EVENT(SDF_EDF_FUNC, TIMER_CALLBACK, 0);

	/* Get reference to the member mem_timer structure */
	md = container_of(mem_timer,  struct edf_member, timer);
	       
	if (unlikely(md->phase == SDF_EDF_MEM_CLEANUP))
		return HRTIMER_NORESTART;
	      
	/*Get reference to the group mem_timer structure from the member*/
	group = md->member->owner;

	gd = (struct edf_group*)group->sched_data;
	
	atomic_spin_lock_irqsave(&group->lock, flags);

	switch (md->phase) {
	case SDF_EDF_MEM_FINISHED:		
		__sdf_edf_set_member_phase(gd, md, SDF_EDF_MEM_READY);
		break;		
		
	case SDF_EDF_MEM_READY:
	case SDF_EDF_MEM_RUNNING:
		DSTRM_EVENT(SDF_EDF, MEM_MISS_DEADLINE, 0);
		__sdf_edf_set_member_phase(gd, md, SDF_EDF_MEM_READY);
		break;
	case SDF_EDF_MEM_SETUP:
	default:
		BUG();
	};


	hrtimer_forward(&md->timer, timespec_to_ktime(md->deadline), 
			timespec_to_ktime(md->period));


	/**
	 * Calculate the deadline of the member as the previous
	 * deadline + period using a function that incorporates the
	 * rollover of carryover nanoseconds to seconds.
	 */
	set_normalized_timespec(&md->deadline,
				md->deadline.tv_sec + md->period.tv_sec, 
				md->deadline.tv_nsec + md->period.tv_nsec);

	atomic_spin_unlock_irqrestore(&group->lock, flags);



	set_need_resched();	
	
	return HRTIMER_RESTART;
}



/**
 * Prepare to make a scheduling decision. This function executes once
 * before iterator_next.
 *
 * @group: The group which is being given a chance to pick a member to run.
 * 
 * @rq: The CFS runqueue for the current CPU.
 */
static void 
sdf_edf_iterator_prepare(struct gsched_group *group, struct rq *rq)
{
	DSTRM_EVENT(SDF_EDF_FUNC, ITERATOR_PREPARE, 0);
	atomic_spin_lock(&group->lock);
}


/** 
 * Calling context holds the group->lock
 */
static __always_inline struct edf_member *
__sdf_edf_find_earliest_deadline(struct gsched_group *group, struct gsched_member *prev,
				 struct edf_cpu *cd, int cpu)
{
	struct edf_group          *gd;
	struct edf_member         *md;
	struct edf_member_cpu     *mem_cpu;
	struct edf_member         *next = NULL;

	gd = (struct edf_group*)group->sched_data;

	DSTRM_EVENT(SDF_EDF_FUNC, FIND_EARLIEST_DL, 0);
	switch(gd->phase) {
	case SDF_EDF_GROUP_SETUP:
		if (prev) {
			md = prev->sched_data;
			mem_cpu = &md->cpu[cpu];

			list_for_each_entry_continue(mem_cpu, 
						     &cd->sorted_members, 
						     sorted_ent) {
				md = mem_cpu->edf_member;
				/* Doesn't matter in which order we
				 * run the setup members until they
				 * get to sync so grab the first and
				 * go with it. 
				 */
				if (md->phase == SDF_EDF_MEM_SETUP) {
					next = md;
					break;
				}
				
			}
		} else {
			list_for_each_entry(mem_cpu, 
					    &cd->sorted_members, sorted_ent) {

				md = mem_cpu->edf_member;
				/* Doesn't matter in which order we
				 * run the setup members until they
				 * get to sync so grab the first and
				 * go with it. 
				 */
				if (md->phase == SDF_EDF_MEM_SETUP) {
					next = md;
					break;
				}
			}
		}
		break;
		
	case SDF_EDF_GROUP_RUNNING:
		if (prev) {
			md = prev->sched_data;
			mem_cpu = &md->cpu[cpu];
			list_for_each_entry_continue(mem_cpu, 
						     &cd->sorted_members, 
						     sorted_ent) {
				md = mem_cpu->edf_member;
				/* Find the member with the earliest
				 * deadline that is runnable within
				 * the current epoch.
				 */
				if ((!next || 
				     timespec_compare(&next->deadline, &md->deadline) > 0) &&
				    md->phase == SDF_EDF_MEM_READY) {

					next = md;
				}
				
			}
		} else {
			list_for_each_entry(mem_cpu, &cd->sorted_members, 
					    sorted_ent) {

				md = mem_cpu->edf_member;

				/* Find the member with the earliest
				 * deadline that is runnable within
				 * the current epoch.
				 */
				if ((!next || timespec_compare(&next->deadline, &md->deadline) > 0) &&
					md->phase == SDF_EDF_MEM_READY) {
					next = md;
				}

			}
		}
		
		break;

	default:
		BUG();
	}

	return next;
}

/**
 * Pick a member which should be scheduled. The member could be a task
 * or a group. If it is a group then these iterator functions will be
 * executed for the group. If the member is a task then group
 * scheduling will attempt to schedule the task. However, scheduling
 * the task is not always successful and sometimes a group will not
 * make a decision.  In these cases, group scheduling will call
 * iterator_next again and another decision must be made.
 *
 * Three return values are possible:
 * &GSCHED_LINUX: Stop group scheduling evaluation and immediately allow
 *                linux(CFS( to make a decision.
 * NULL: Indicate that no decision can be made.
 * A member structure: The member group scheduling should evaluate next.
 *
 * @group: The group being iterated over.
 *
 * @prev: The last decision which was made by this group. It will be NULL
 *        on the first time iterator_next is called.
 *
 * @rq: The CFS runqueue for the current CPU.
 */
static struct gsched_member *
sdf_edf_iterator_next(struct gsched_group *group, struct gsched_member *prev,
		       struct rq *rq)
{

	struct edf_cpu *cd;
	struct edf_member *next = NULL;
	int cpu;

	DSTRM_EVENT(SDF_EDF_FUNC, ITERATOR_NEXT, 0);

	cpu = smp_processor_id();
     
	/* Get the member list only for the current cpu */
	cd = (struct edf_cpu*)group->cpu_sched_data[cpu];

	next = __sdf_edf_find_earliest_deadline(group, prev, cd, cpu);

	if (!next) {
		DSTRM_EVENT(SDF_EDF, SCHED_CHOICE_NULL, 0);
		return NULL;
	}

	DSTRM_EVENT_PRINTF(SDF_EDF, SCHED_CHOICE, 0, "%s", 
			   next->member->ns->name);
	
	return next->member;       
}

/**
 * iterator_finalize is called when a member picked by a group has
 * been successfully evaluated. It allows the SDF to perform any
 * necessary cleanup.
 *
 * @group: The group which has successfully picked a member for
 *         group scheduling to evaluate.
 * @next: The member which was picked by the group.
 * @rq: The runqueue for CFS.
 *
 */
static void sdf_edf_iterator_finalize(struct gsched_group *group,
				  struct gsched_member *next, 
				  struct rq *rq)
{
	DSTRM_EVENT(SDF_EDF_FUNC, ITERATOR_FINALIZE, 0);

	atomic_spin_unlock(&group->lock);
}


/**
 * insert_group is called when a group is created and specifies that
 * it would like to be controlled by this sdf.  The group->lock is
 * acquired before calling this function, by group scheduling, and the
 * group structure is freely accessible without any concurrency
 * control.
 *
 * @group: The group which is being placed under control of this SDF.
 */
static void sdf_edf_create_group(struct gsched_group *group)
{
	struct edf_cpu *cd = NULL;
	struct edf_group *gd = NULL;
	int cpu;

	DSTRM_EVENT(SDF_EDF_FUNC, CREATE_GROUP, 0);       

	/* Setup of the groups CPU lists */
	for_each_possible_cpu(cpu) {
		cd = (struct edf_cpu*)group->cpu_sched_data[cpu];

		INIT_LIST_HEAD(&cd->sorted_members);
	}

	/* Initialization of the group's data */
	gd = (struct edf_group*)group->sched_data;
	gd->group = group;
	gd->num_mems = 0;
	gd->__mem_count = 0;
	gd->__ready_count = 0;
	gd->phase = SDF_EDF_GROUP_SETUP;
}


/**
 * setup_member is called when a member is being allocated by group
 * scheduling. The member has not actually ben added to the given
 * group but group scheduling knows that it is about to add the member
 * to the group. The advantage of this function is that the group->lock
 * is not held and blocking operations can be performed. However,
 * the group structure should not be accessed because the group->lock
 * is not held.
 *
 * @group: The group that the member will be added to later on.
 * @member: The member structure to initialize.
 */
static void sdf_edf_setup_member(struct gsched_group *group, 
                             struct gsched_member *member)
{

	struct edf_member* md = NULL;
	int cpu;

	DSTRM_EVENT(SDF_EDF_FUNC, SETUP_MEMBER, 0);

	md = (struct edf_member*)member->sched_data;

	md->member   = member;
	md->deadline = SDF_EDF_DEFAULT_DEADLINE;	
	md->period   = SDF_EDF_DEFAULT_PERIOD;
	md->phase   = SDF_EDF_MEM_SETUP;

	/* Initialize release time hrimer here */
	hrtimer_init(&md->timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
	md->timer.function = sdf_edf_member_deadline_timer_callback;      

	for_each_possible_cpu(cpu) {
		INIT_LIST_HEAD(&md->cpu[cpu].sorted_ent);
		md->cpu[cpu].edf_member = md;
	}
	
}

/**
 * Insert a member into the appropriate per cpu member list using the
 * deadline argument to determine where the member is inserted in the
 * list. The per cpu member list is sorted in increasing deadline
 * order.
 *
 * Note that this uses the deadline argument for sorted insert instead
 * of using the "deadline" within the member data. This is to overload
 * the semantics of the list sorting for cases when sorted insert does
 * not make sense. 
 *
 * Take, for example, the case where a group is waiting for all of the
 * members to enter the "ready phase". Before members are released,
 * their deadlines will have the value of 0sec 0nsec as the first time
 * the deadline is set is when all of the members become ready and the
 * group releases all of them. In this case, instead of reordering the
 * list at barrier release time, we just insert the members into the
 * group by period instead of deadline when their period is set during
 * the setup phase.
 * 
 * Calling context must hold the group->lock.
 */
static void __sdf_edf_sorted_insert(struct list_head *ent,
				    struct list_head *list,
				    struct timespec *deadline)
{
	struct list_head *insert_before;
	struct edf_member_cpu *iter;

	DSTRM_EVENT(SDF_EDF_FUNC, SORTED_INSERT_PRIV, 0);

        insert_before = list;

        list_for_each_entry(iter, list, sorted_ent) {
                if (timespec_compare(deadline, &iter->edf_member->deadline) > 0) {
                        insert_before = &iter->sorted_ent;
                        break;
                }
        }

        /*
	 * List_add_tail will place the element before the destination element
	 * becuase it links it into the tail (prev) pointer of the destination.
         * If the new member is greater than all of the other members then     
         * it will get insrted at the tail of the list.     
         */
        list_add_tail(ent, insert_before);

}

/**
 * Insert the member into the appropriate per CPU member list (for
 * tasks) or all CPU member lists (for groups) using the deadline
 * argument for a sort comparitor. 
 *
 * See Also: __sdf_edf_sorted_insert()
 *
 * Calling context holds group->lock
 * 
 * Only called by EDF internally
 *
 */
static void __sdf_edf_insert_member(struct gsched_group *group,
				    struct gsched_member *member,
				    struct timespec *deadline)
{
	struct edf_cpu *cd;
	struct edf_group *gd;
	struct edf_member *md;
	int cpu;

	DSTRM_EVENT(SDF_EDF_FUNC, INSERT_MEMBER_PRIV, 0);

	gd = (struct edf_group*)group->sched_data;
	md = (struct edf_member*)member->sched_data;
	
	switch(member->type) {
	case GSCHED_COMPTYPE_TASK:
		cpu = gsched_task_cpu(&member->task);

		cd = (struct edf_cpu*)group->cpu_sched_data[cpu];		

		__sdf_edf_sorted_insert(&md->cpu[cpu].sorted_ent, 
					&cd->sorted_members, deadline);

		DSTRM_EVENT_PRINTF(SDF_EDF, INSERT_TASK, 0, 
				   "%d|%s", cpu, member->ns->name);

		break;
	case GSCHED_COMPTYPE_GROUP:
		for_each_possible_cpu(cpu) {
			cd = (struct edf_cpu*)group->cpu_sched_data[cpu];

			__sdf_edf_sorted_insert(&md->cpu[cpu].sorted_ent,
						&cd->sorted_members, deadline);
		}
		break;
	default:
		BUG();
	};
}


/**
 * Calling context holds group->lock.
 *
 * Called from hgs API level.
 */
static void sdf_edf_insert_member(struct gsched_group *group,
				  struct gsched_member *member)
{	
	struct edf_group   *gd;
	struct edf_member  *md;

	DSTRM_EVENT(SDF_EDF_FUNC, INSERT_MEMBER, 0);

	gd = (struct edf_group*)group->sched_data;
	gd->__mem_count++;

	md = (struct edf_member*)member->sched_data;
	

	DSTRM_EVENT(SDF_EDF, INC_MEM_COUNT, gd->__mem_count);

	__sdf_edf_insert_member(group, member, &md->period);

		
}

/**
 * Calling context holds group->lock
 * 
 */
static void __sdf_edf_remove_member(struct gsched_group *group,
				  struct gsched_member *member){

	struct edf_member *md;
	struct edf_group *gd;
	int cpu;
	
	DSTRM_EVENT(SDF_EDF_FUNC, REMOVE_MEMBER_PRIV, 0);

	gd = (struct edf_group*)group->sched_data;
	md = (struct edf_member*)member->sched_data;
  
	switch(member->type) {
	case GSCHED_COMPTYPE_TASK:
		cpu = gsched_task_cpu(&member->task);
		list_del(&md->cpu[cpu].sorted_ent);		
		break;
	case GSCHED_COMPTYPE_GROUP:
		for_each_possible_cpu(cpu) {
			list_del(&md->cpu[cpu].sorted_ent);
		}
		break;
	default:
		BUG();
	};

}



/**
 * TODO: Does calling context also hold the gsched_lock?
 * Calling context holds group->lock
 * 
 * remove_member is called when a member is removed from the group.
 * This can occur either because of an explicit call to group
 * scheduling or implicitly due to a task dying.  The group->lock is
 * acquired before this function is called and this function cannot
 * perform blocking operations.  The group structure can be freely
 * accessed.
 *
 * @group: The group the member is being removed from.
 * @member: The member that is being removed.
 */
static void sdf_edf_remove_member(struct gsched_group  *group,
				  struct gsched_member *member)
{
	struct edf_group *gd;
	struct edf_member *md;

	DSTRM_EVENT(SDF_EDF_FUNC, REMOVE_MEMBER, 0);

	gd = (struct edf_group*)group->sched_data;
	md = (struct edf_member*)member->sched_data;

	gd->__mem_count--;
	DSTRM_EVENT_PRINTF(SDF_EDF, DEC_MEM_COUNT, 0, "%d", gd->__mem_count);

	__sdf_edf_set_member_phase(gd, md, SDF_EDF_MEM_CLEANUP);
	__sdf_edf_remove_member(group, member);
		
}

/**
 * Calling context holds **NO** locks.
 *
 */
static void sdf_edf_release_member(struct gsched_group  *group,
				  struct gsched_member *member)
{
	struct edf_member    *md;

	md = (struct edf_member*)member->sched_data;

	DSTRM_EVENT(SDF_EDF_FUNC, RELEASE_MEMBER, current->pid); 

	hrtimer_cancel(&md->timer);	
}


/**
 * Calling context holds the group->lock
 * 
 * Called by GS when a task switches a CPU or its
 * proxy switches CPUs.
 *
 * @param group Group that member belongs to.
 * @param member Member that is being moved. It will always be a task.
 * @param oldcpu The cpu from which it is moving.
 * @param newcpu The cpu to which it is moving.
 */
static void sdf_edf_move_member(struct gsched_group *group,
				struct gsched_member *member,
				int oldcpu,
				int newcpu) {
	struct edf_member *md;
	struct edf_cpu *cd;

	DSTRM_EVENT(SDF_EDF_FUNC, MOVE_MEMBER, 0);

	md = member->sched_data;
	cd = group->cpu_sched_data[newcpu];

	list_del(&md->cpu[oldcpu].sorted_ent);
	__sdf_edf_sorted_insert(&md->cpu[newcpu].sorted_ent, 
				&cd->sorted_members, &md->deadline);

	DSTRM_EVENT_PRINTF(SDF_EDF, MOVE_MEMBER, 0, "%d|%d|%s", oldcpu, newcpu,
			   member->ns->name);
}


/**
 * Note: This is bogus, you have to populate the passed param
 *     structure as you would populate the edf_member struct due to the
 *     API automatically passing in void pointer that refers to a
 *     structure that is sizeof(struct edf_member).
 *     - Kept for reference.
 */
static int
sdf_edf_get_member_params(struct gsched_group *group,
			  struct gsched_member *member, 
			  void *param, size_t size) {
	struct sdf_edf_mp *mp;
	struct edf_member *md;
	unsigned long flags;
	int ret = 0;


	if (!param || size != sizeof(struct sdf_edf_mp))
		return -EINVAL;

	mp = (struct sdf_edf_mp*)param;
	      
	md = (struct edf_member*)member->sched_data;

	
	switch(mp->cmd) {
	case SDF_EDF_MP_CMD_PERIOD:
		atomic_spin_lock_irqsave(&group->lock, flags);
		mp->period = md->period;
		atomic_spin_unlock_irqrestore(&group->lock, flags);
		break;

	case SDF_EDF_MP_CMD_PHASE:
		atomic_spin_lock_irqsave(&group->lock, flags);
		mp->phase = md->phase;
		atomic_spin_unlock_irqrestore(&group->lock, flags);
		break;
	default:	    
		ret = -EINVAL;
	};


	return ret;
}

/**
 * Calling context holds the group->lock
 *
 * 
 * Note that this may be a place where we will need to relase all
 * members which may be on different CPUS. 
 */
static void sdf_edf_release_all_members(struct edf_group *gd) {

	struct gsched_member *member;
	struct gsched_group  *group;
	struct edf_member    *md;
	struct timespec      now_time;

	
	group = (struct gsched_group*)gd->group;

	DSTRM_EVENT(SDF_EDF_FUNC, RELEASE_ALL_MEMBERS, 0);

	/* Get the current ktime */
	ktime_get_ts(&now_time);

	list_for_each_entry(member, &group->all_members, owner_entry) {
		
		md = (struct edf_member*)member->sched_data;
		DSTRM_EVENT(SDF_EDF, BARRIER_RELEASE, 0);
		__sdf_edf_set_member_phase(gd, md, SDF_EDF_MEM_READY);
		DSTRM_EVENT(SDF_EDF, START_MEM_TIMER, 0);

		/* Calculate the deadline of the member as now +
		 * period using a function that incorporates the
		 * rollover of nanoseconds to seconds. 
		 */
		set_normalized_timespec(&md->deadline,
					now_time.tv_sec + md->period.tv_sec, 
					now_time.tv_nsec + md->period.tv_nsec);
		hrtimer_start(&md->timer, timespec_to_ktime(md->deadline), HRTIMER_MODE_ABS);
	}

}



/**
 * Calling context holds the group->lock
 *
 */
static void __sdf_edf_set_member_phase(struct edf_group *gd, 
					struct edf_member *md, 
					int next_phase) {
	
	int prev_phase = md->phase;

	DSTRM_EVENT(SDF_EDF_FUNC, SET_MEMBER_PHASE, 0);

	DSTRM_EVENT_PRINTF(SDF_EDF, SWITCH_PHASE, 0, "%d->%d", 
			   prev_phase, next_phase);

	if (prev_phase == SDF_EDF_MEM_SETUP &&
		next_phase == SDF_EDF_MEM_FINISHED ) {

		gd->__ready_count++;

		DSTRM_EVENT(SDF_EDF, INC_READY_COUNT, gd->__ready_count);
		
		if (gd->phase == SDF_EDF_GROUP_SETUP && 
		    gd->__ready_count == gd->num_mems) {
			gd->phase = SDF_EDF_GROUP_RUNNING;
			DSTRM_EVENT(SDF_EDF, GROUP_RUNNING, 0);
			sdf_edf_release_all_members(gd);
			return;
		}	
	}

	md->phase = next_phase;
}

/**
 * Calling context does not hold any hgs locks.
 *
 */
static int
sdf_edf_set_member_params(struct gsched_group *group,
			  struct gsched_member *member, 
			  void *param, size_t size)
{
	struct sdf_edf_mp *mp;
	struct edf_member *md;
	unsigned long flags;
	int ret = 0;
	int resched = false;

	DSTRM_EVENT(SDF_EDF_FUNC, SET_MEMBER_PARAMS, 0);

	mp = (struct sdf_edf_mp*)param;
	md = (struct edf_member*)member->sched_data;

	if (!param || size != sizeof(struct sdf_edf_mp))
		return -EINVAL;


	switch (mp->cmd) {
	case SDF_EDF_MP_CMD_PERIOD:

		DSTRM_EVENT_PRINTF(SDF_EDF, SET_PERIOD, 0, "%s|%ld|%ld", 
				   member->ns->name, mp->period.tv_sec, 
				   mp->period.tv_nsec);

		atomic_spin_lock_irqsave(&group->lock, flags);
		md->period = mp->period;
		if (md->phase == SDF_EDF_MEM_SETUP) {
			/* If in the "setup phase" sync the members
			 * based on their period because there is no
			 * start time and no deadline set yet.
			 * 
			 * This may be made more efficient by doing
			 * a move instead of deleting and then re-inserting.
			 */			
			__sdf_edf_remove_member(group, member);
			__sdf_edf_insert_member(group, member, &md->period);
			resched = true;
		}
		
		atomic_spin_unlock_irqrestore(&group->lock, flags);


		break;
	case SDF_EDF_MP_CMD_PHASE:
		
		atomic_spin_lock_irqsave(&group->lock, flags);
		__sdf_edf_set_member_phase(
			(struct edf_group*)group->sched_data, md, mp->phase);
		atomic_spin_unlock_irqrestore(&group->lock, flags);

		resched = true;

		break;
	default:
		ret = -EINVAL;
	}	      

	if (resched)
		set_need_resched();

	return ret;
}


/**
 * Note: This is bogus, you have to populate the passed param
 *     structure as you would populate the edf_group struct due to the
 *     API automatically passing in void pointer that refers to a
 *     structure that is sizeof(struct edf_group).
 *     - Kept for reference.
 */
static int
sdf_edf_get_group_params(struct gsched_group *group, void *param, size_t size)
{
	struct sdf_edf_gp *gp;
	struct edf_group *gd;
	unsigned long flags;
	int ret = 0;

	if (!param || size != sizeof(struct sdf_edf_gp))
		return -EINVAL;

	gp = (struct sdf_edf_gp*)param;
	      
	gd = (struct edf_group*)group->sched_data;

	
	switch (gp->cmd) {
	case SDF_EDF_GP_CMD_NUM_MEMS:
		atomic_spin_lock_irqsave(&group->lock, flags);
		gp->num_mems = gd->num_mems;
		atomic_spin_unlock_irqrestore(&group->lock, flags);
		break;

	case SDF_EDF_GP_CMD_MEM_COUNT:
		atomic_spin_lock_irqsave(&group->lock, flags);
		gp->mem_count = gd->__mem_count;
		atomic_spin_unlock_irqrestore(&group->lock, flags);
		break;
	default:	    
		ret = -EINVAL;
		break;
	}

	
	/* Change this to ret when this method becomes not so bogus */
	return -EINVAL;
}


/**
 * Calling context holds no locks.
 * 
 * Set the parameters for a group using this sdf.
 * The group->lock is not held.
 *
 * @group: The group to set data on.
 * @param: Typcially a structure defined by this sdf specifically
 *         designed to carry data from user space to kernel space.
 * @size: The size of param.
 */
static int sdf_edf_set_group_params(struct gsched_group *group,
                                void *param, size_t size)
{
	struct sdf_edf_gp* gp;
	struct edf_group* gd;
	unsigned long flags;
	int ret = 0;

	DSTRM_EVENT(SDF_EDF_FUNC, SET_GROUP_PARAMS, 0);

	if (!param || size != sizeof(struct sdf_edf_gp)) {
		return -EINVAL;
	}

	gp = (struct sdf_edf_gp*)param;

	gd = (struct edf_group*)group->sched_data;

	switch (gp->cmd) {
	case SDF_EDF_GP_CMD_NUM_MEMS:

		atomic_spin_lock_irqsave(&group->lock, flags);
		gd->num_mems = gp->num_mems;
		DSTRM_EVENT(SDF_EDF, SET_NUM_MEMS, gd->num_mems);
		atomic_spin_unlock_irqrestore(&group->lock, flags);

		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


/*
 * This structure identifies the hoocks and other information
 * about the SDF to group scheduling. It is sent to group scheduling
 * when the module is loaded.
 */
struct gsched_sdf sdf_edf = {
	.name = SDF_EDF_NAME,
	.owner = THIS_MODULE,

	.find_member = gsched_default_find_member,
	
	
	.iterator_prepare  = sdf_edf_iterator_prepare,
	.iterator_next     = sdf_edf_iterator_next,
	.iterator_finalize = sdf_edf_iterator_finalize,
	
	.create_group  = sdf_edf_create_group,
	.setup_member  = sdf_edf_setup_member,
	.insert_member = sdf_edf_insert_member,
	.remove_member = sdf_edf_remove_member,
	.release_member = sdf_edf_release_member,
	.move_member = sdf_edf_move_member,

	.set_member_params = sdf_edf_set_member_params,
	.get_member_params = sdf_edf_get_member_params,

	.set_group_params = sdf_edf_set_group_params,
	.get_group_params = sdf_edf_get_group_params,

	.per_member_datasize = sizeof(struct edf_member),
	.per_group_datasize = sizeof(struct edf_group),
	.per_cpu_datasize  = sizeof(struct edf_cpu),

};

/* Called on module install. Registers the SDF with the Group
 * Scheudling framework so that new groups may attempt to register
 * this SDF.
 */
static int __init sdf_edf_init(void)
{
	gsched_register_scheduler(&sdf_edf);
	return 0;
}

/*  
 * Called on module uninstall. Unregisters the SDF from the Group
 * Scheduling framework so that new groups may not attempt to register
 * this SDF.
 */
static void __exit sdf_edf_cleanup(void)
{
	gsched_unregister_scheduler(&sdf_edf);
}


module_init(sdf_edf_init);
module_exit(sdf_edf_cleanup);

MODULE_LICENSE("GPL");
