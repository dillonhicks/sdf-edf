
Earliest Deadline First SDF
===========================

General Discussion
******************

- Schedules periodic tasks based on a deadline

  - Candidate task is chosen from the set of tasks such that it is the
    task with the earliest deadline.

  - If the task is otherwise not runnable the task with the next
    earliest deadline should be chosen.

    - The task with the earliest deadline may not be chosen because of
      the following reasons:


      - The task has finished its execution before its current
        deadline and should not be allowed to run again until after
        its deadline has passed.

        - We shall consider this time to be the *release time*

	- To control entropy/reduce initial complexity we shall
          consider the deadline of the current execution period to be
          the *release time* for the task.

        - A future version of edf would likely not adhear to these
          limitations.

- Given the fact that all tasks and members must have a deadline we must:

  - Have some parameter for users to specify deadlines

  - Have an OS mechanism for alerting the SDF of the passing of a
    deadline.

    - HRTIMERS are likely the only suitable mechanism to get precision
      required for realtime systems..

      - They typically work on nanosecond time scales (ns) which is
        the finest grain timescale that is capable with generic
        hardware. These values use the HPET hardware which gaurentees
        the timer clock to have a frequency of 10MHz which would put
        the lowest granularity of the timers we wish to use at about
        100 ns.

      - That being said, we will likely want to support a range of
        timescales.

        - This should be accomplished by using TIMESPEC time
          representation and the appropriate conversion functions.

      - Pitfall: HRTIMERS must be canceled before the member is
        released so that the timer callback does have a null pointer
        dereference within the kernel.

- We wish that all the threads execute on the same time line, for that
  we will need some type of thread syncronization.

  - While this could be done with pthread sync barriers, we are
    attempting to use EDF to demonstrate the edf programming model so
    that we should use the semantics of the EDF to provide a
    syncronization barrier.

    - A sample implementation would be to give a group datum for the
      expected number of member.

    - Upon a member task or group entering a particular EDF would then
      update an internal current member count.

    - Once all expected members have arrived (expected members ==
      current members) and they have all set their deadlines, the per
      member timers would then be started and all members status
      should indicate that they are ready for workloop execution.


EDF Test Program Flow
*********************

  - Summary:

    - A good (initial) test program should spawn multiple tasks, have the task
      register themselves with CCSM, set their deadlines, execute the
      per task workloop a finite amount of times, and exit.

      - Note that in rt/embedded systems it is likely that these
        processes would likely not exit while the system was up. For
        testing purposes it is impossible to assume that we should
        desire infinitely running processes, but we should pick a
        sufficiently large interval for a reasonable time slice to
        fully examine application and kernel behavior.

  - Pre-execution 

    - Use *gschedexec* along with a .hgs configuration file to setup a
      CCSM representation of the heirarchy.
 
      - When tasks register themselves with CCSM they will then be
        inserted into the heirarchy appropriately.

    - *dskictrl* should execute the program with the arguments to
       *gschedexec* and turning on the appropriate instrumentation
       points within the kernel.

    - A postprocessing .pipes file should at minimum give a narrative
      of the periodic execution.

    - The user application should be instrumented with datastreams
      (DSUI) such that the postprocessing narrative will tell us:

      - Begin and end points of overall per thread execution
      - Begin and end times of per thread workloop execution intervals
      
    - Similarly the Kernel DSKI instrumentation should give us:

      - CCSM Events that tell us that threads have registered and the
        heirarchy is being assembled successfully.

      - Give us the scheduling decisions for each thread.

        - Because of nuiances we would likely also like to know why. 
  
          - Giving the tasks status flag within the IP should be useful
	  - Task deadline 

      - Give deadline changes that would occur using the hgs api IOCTL
        calls.

      - Also we would like to measure the per task execution time in
        the kernel as this could be useful for showing:

        - If a task was prempted by a released task that had an
          earlier deadline. 
	  	  
          - How this effects the overall execution time of the preempted task.
        
      - IPs for when a tasks missed its deadline and the execution
        time until the deadline was missed.

  - Per task code execution

    - Use the CCSM calls to register each task

      - Rendezvous should then:

        - Create the proper HGS group if it does not exist. 

        - Insert the member corresponding to the task into the proper
          HGS group.



Total Flow
**********

- dskictrl:

    #. Intializes the DSKI context using the dski configuration file
       for the test program.
    #. Executes gschedexec, which will execute the test program.
    
- gschedexec:

    #. Uses the HGS API to create a HGS hierarchy from an .hgs
       configuration file.
    #. Executes edf_user_prog with \-\-config=<file> argument as the
       path to the XML configuration file that specifies the
       parameters for each member thread.

- edf_user_prog:

    #. Parse configuration file given with the commandline argument
       \-\-config=<file> and stores the pertinant information from the
       configuration file in a global structure named "Params".

        #. Stores information about each thread specified in the XML
           configuration file as a **threadspec** linked list
           structure.
	   
    #. The executive thread sets the expected number of members for
       the EDF group which is used for barrier sync.

    #. The executive thread then iterates over the threadspec
       structures in Params and spawns a thread for each threadspec,
       sending the thread code function a pointer to the corresponding
       threadspec. 

       The newly spawned thread then:

        #. Sets its CPU assignment with sched_setaffinity with the cpu
           field in targs.

        #. The thread announces its name to CCSM using
           ccsm_create_component_self() and supplying it with the name
           given in targs.

	    - This in turn will triger the ccsm ".bind" callback which
              will call the callback defined in hgs_core.c and take
              the placeholder member and from it create a task member
              and place it within the EDF group.

	    - The member that is created from the bind will, by
              default, have:

	        - period = 0s 0ns
		- phase  = SDF_EDF_MEM_SETUP		

        #. The thread code then uses attributes in the threadspec to
           call the EDF api provided by libsdf_edf to set its period
           **sdf_edf_set_member_period()**.

        #. The thread code then drops into its workloop and sets the
           task's member phase to **SDF_EDF_MEM_FINISHED** via a call
           to **sdf_edf_next_instance()**

	      - The byproduct of this library routine is the internal
                EDF group **__ready_count** count is incremented. 
		
	      - If **__ready_count** == **num_mems** the thread then
                stays in the **SDF_EDF_MEM_FINISHED** state and will
                not be chosen to run until **__ready_count** ==
                **num_mems**. This will mean that all of the expected
                threads for the group have been spawned, announced
                their names to CCSM, became a task member for the HGS
                group due to CCSM reundevous with HGS, set their
                period, and have reached the sync barrier (which is
                considered to be the first call to
                **sdf_edf_next_instance()**.

	      - Otherwise the group's **__ready_count** = **num_mems**
	        then task executing the call will first get set the
	        group's phase to **SDF_EDF_GROUP_RUNNING**, get the
	        current kernel time (named **now**) to be used as the
	        absolute timeline, and iterate over all of the members
	        in the EDF group such that for each member it will:

	          - Change the phase of the member to
                    **SDF_EDF_MEM_READY** to signify that it is now
                    choosable to run by the EDF scheduler.

		  - Start the member's hrtimer giving it the
                    expiration time such that, expiration = **now** +
                    **period** where **now** is the previously
                    mentioned *current* kernel time and **period** is
                    the period attribute of the member.

		    
	   
	#. T
