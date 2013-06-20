 
Admiral Henderson, Commander of the 7th Fleet, Terror of K10, Crusher of Weak Souls
====================================================================================

- Reading List:

   - http://www.gnu.org/s/hello/manual/libc/Signal-Handling.html#Signal-Handlignc
   - sdf_edf/requirements.rst


- We need to start renaming everything with a "gsched" to "hgs"

    - I suggest we keep two versions of user libraries in parallel
      until we consider the new "hgs" one to be complete then revise
      all of our old examples.

    - Examples we create during this time would, by self enforcement,
      only be allowed to use the newly named libraries.

- [DONE] Remove the cast warning from edfup0 if able. 

   - Most of the errors about about xmlChars, but if I am not mistaken
     those are just typedefs::

         typedef unsigned char xmlChar

     So the warnings are not critical, but doug will not like them and
     they can cause a bit of confusion when I just want to see
     errors. So I would recommend just changing::

         atio(xml_char_datum);

     To something similar to::
     
         atoi((const char*)xml_char_datum);
         

- [DONE] Create a .dsui configuration file so that our datastreams binary will
  be logged to the test build directory instead of /tmp.

    - This is then a command line arg --dsui-config=<config> that is
      automatically parsed by DSUI (may not be 100% correct, this is
      from memory.).


- Discuss with dillon the consideration about having
  gschedexec/gschedctrl set the member parameters from a group
  scheduling configuration file. These .gsh files should be found in
  the edfup0 as well as many of the example directories that are
  suffixed with "_gschedctrl." Currently this will only setup a CCSM
  set(s) representing the eventual HGS heirarchy without worrying
  about inital member parameters.

    - You may find the config language stupid, which it likely is as
      it is/was:

        - One of my first projects with kusp
	- Has not been changed in over two years
	- Likely needs expansion and revision due to changes in the
          hgs API.
	- Likely would need replacement with a suitable xml based
          configuration language.

    - Setting the inital parameters of groups would require python
      code to use the IOCTL calls to set C member parameters
      attributes. Tyrian and Dillon developed decent way to do this
      last summer using swig to wrap user level C code. 

        - Notes and Pitfalls to SWIG

	    - There are some considerations that must be made as not
              to use void pointers which will confuse swig.

	    - Also if you pass a binary string packed with the Python
              struct module to one of these IOCTL call wrappers, You
              must make sure that the structure is the same size as
              the C structure. 

	        - GCC likes to pad structures so that they come out to
                  multiples of 32 or 64 bits. This was an issue tyrian
                  observed for passing parameter structurs to
                  SDF_GUIDE from python to the kernel via swig wrappers. 


- Also, is there a better way to wrap our C libraries for Python (or
  other languages)?

    - Benefit of swig is that it wrapps for a wide range of other
      languages so is preferable in that regard.

      - Considering that we only use C/C++/Java/Python this may not be
        practical if there is a hipper-cooler-simpler-less-pitfally
        way of wrappng C for Python.

    - It is also relatively simple to create wrappers for most cases,
      with swig.

- Consider what would be appropriate for a DSUI narrative at each
  varying stages of refinement that we have planned for the edf user
  programs.

    - Note that this may require you adding the pertinent DSUI 

    - First we will just want to verify order and completeness of
      thread execution.

        - IE. A file with Thread started-work and thread ended
	
	- Possible output from such a file verifying that threads ran
          sequentially in order to completion::

	    THREAD/START [t-1]
	    THREAD/END [t-1]
	    THREAD/START [t-2]
	    THREAD/END [t-2]
	    THREAD/START [t-3]
	    THREAD/END [t-3]

	
	- Possible output from a file showing that we fucked up::

	    THREAD/START [t-1]
	    THREAD/START [t-2] <--- AWWW SHIT something got preempted
	    THREAD/END [t-2]
	    THREAD/END [t-1]
	    THREAD/START [t-3]
	    THREAD/END [t-3]


    - We will want to add some more narrative to this as we consider
      the additions for the future iterations of the user programs
  

        - System Global and Per CPU time lines
	
   	    - User level
          
                - Thread begin/end execution  
	       
                - Per thread work-loop begin/end execution

	        - Narrative for signal handling for missed deadlines
                  and the choice that was taken to resolve the
                  deadline.

            - OS Level (Instrumentation you could do as well if you are comfortable)
	    
	        - Iterator_next
		
		    - Events for comparisons of the scheduling choices 

		        - When a new best choice is picked show
                          deadline

			- If would otherwise have a better deadline,
                          also display the status flag


		- HRTimer callback narrative

	- Use the previous filters to show the CCSM and HGS hierarchies (.png) 

	
	- Once we look like we are on a roll with EDF we would want to
          consider what it would take to make a time-line
          visualization filter.


- Consider what it would take to take our sample XML based
  configuration language to the next level of generalization.

    - We will likely need to discuss Doug the implications of this as
      we will need a way to make the resulting structure from our
      general parser something that is a data structure we prefer, but
      isn't the crazy XML data structure. There may already be libraries
      for this. What I find the least appealing about the XML
      configuration parsing is that we would have to write per
      application XML parsing code.

    - My guess is that we can find a more preferable way that would be
      (mostly) application ambivalent::

          XMLFILE -> libxml2-struct -> general-kusp-struct

      It is possible that not all applications would conform to our

    - My guess this would be a slightly more complexity structure that implements: 
    
       - Hash-tables
       - Linked lists
       - Some support structure 	


    - Convertables:
   
        - .dski

	    - dskictrl
	    
	- .hgs or .gsh

	    - gschedctrl
	    - gschedexec

	- .dsui
	
  	    - Used by libdsui internals (90% sure)
	
	- .pipes or .pp
	
	    - Postprocessing

	- .nspec

	    - Netspec



