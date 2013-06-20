EDF 0: Sequential with dummy API calls
    
        - Test Program - **cpu-bound-workloops**
	
            - Forks **XX** threads and each thread will have:

	        - A number of Execution Periods
		- A number of workloops 
		- For example::
		
		    int thread_run(<args>){
		        ...
		        while(current_periods < periods){
			    sdf_edf_sched_next_instance();
			    for(workloops=0; workloops<max_workloops; workloops++){
			        ; /* Nothing just */
			    }
			}
			...
		    }


        - barrier sync    
		
- User Program 0: CPU Bound Threads 

    - UP 0.0:

       - 5 threads 1 cpu 
       - Use DSPP to check that they are exec-ed in order

    - UP 0.5:
    
       - 5 threads per cpu on at least 2 cpus
       - Check that they are done in order on each CPU
 

- User Program 1: CPU Bound, but put in some nano sleeps so that you
  have 2,3,5 execution intervals for each task. So we can check
  behavior on the previous scenarios so that we can see the
  blocking. Have the tasks announce at the user level that the thread
  is about to sleep to check if the code is able to execute with
  blocking properly.

  - Tune nsleeps so that the sleep << execution period.


- EDF 1: Suggest that we convert the static priorities to be time
  values (timespecs) but make them really big. Need to look into
  timespec->ktime conversion.


  - To reproduce previous behavior but now use the timespec deadlines
    but now assign the timespec deadlines to be big enough and far
    enough apart so that the user programs

   - The deadlines are far enough apart that the threads can run to
     completion in order.

   - For threads A,B,C,D each take 10 time units so give A dl of 20, B
     dl of 50, c dl of 100, D a dl of 150, so that they are
     ridiculously far apart and we can verify that the previous
     semantics are correct.

   - User program 1 with generous deadline, so that the guys with the
     deadlines fall asleep then the guy with the next closest deadline
     will be run.

- EDF 2: Actually add an HRTimer to detect deadline violation

- User Program 2: Where we will set the deadlines, have the ABCD
  threads as before but have one of them execute longer than their
  deadline. The easier way is to make an infinite loop, but a safer
  way would be to make one have a huge work loop. Such that it will
  exceed even the most generous deadlines.

  - That will let you check that the HRTimer is going off correctly so
    that we can verify the the timer check for deadline. This will
    likely need to set a member flag to show the state of the member
    and the fact that it has exceeded its deadline.

  - If everything is working well, make sure to cancel the HRTimers
    correctly.

- User Program 3: This is where we would need to figure out how to
  send the signal to the user program from inside the kernel to the
  program when a task has exceeded its deadline. Kill system call
  would be the way a user program would send a signal, this would
  allow us to see the best way to send a signal to the user program
  from the kernel. Doug suggest that we use "siguser1". We would then
  need to modify the user level code to have a signal handler for
  siguser1 signals.

  - The deadline violation routine, exit with some data streams
    instrumentation narrative.

- EDF 2.5: We will then graduate to more than one instance of each
  period of execution for each thread. I.E. The outer while work loop.

- User Program 4: First realistic program. Each thread sets its
  scheduling criteria that was our previous model.

  - Period WCET
  - Deadline
  - Multiple Instances (while work loop)

- EDF 3: Start specifying relative deadlines within an epoch. Therefore
  you need HRTIMERs for deadline violation separate from release time
  timers. Further you will have to cancel the deadline watchdogs,
  unless deadline violation has occured.

GENERAL EDF NOTES:

- Add Barrier sync so that edf will require all of the threads that
  its been told about announce themselves and enter the barrier sync
  call before the edf will schedule any of the expected threads. 

- Different types of workloops:

    - Standard Tunable work loop
    - Tunable work loop with tunable sleeps
    - A near-infinite work loop that will blow the deadline


- Possible enhancement to the usersig1: If you send it a signal
  because of a dl violation, you can do a long jump back to the
  routine and may continue the workloop routine.

Terms:

- (Task) Instance: An execution of a task within a specific
     epoch. Instance-1, Instance-2, Instance-3 corresponds to
     execution within Epoch-1, Epoch-2, Epoch-3 respectively.

- (Task) Period: Interval of task repitition. (I.E 1 per X time units,
     eg once per second).

- (Task) Epoch: A specific interval of task execution. I.E. Epoch-1,
     Epoch-2, Epoch-3

- (Absolute task) Deadline: The time by which a specific instance of a task
     must complete.
 
- (Relative task) Deadline: The offset within an epoch by which the
     task must complete, therefore it is offset from the begining of
     the epoch..

- Release Time (of a task): Is the begining of the corresponding task
     epoch. E.g. The relase time of task instace 5 is the begining of
     task-epoch-5

- SIGUSER1  10


- kill -l: will show you all of the parameters of all of the available
  signals.
