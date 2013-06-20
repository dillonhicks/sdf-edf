New SDF Checklist
=================

- When writing an SDF you will need to have the following components
  as a minimum. **(Some exceptions will apply)**

    - SDF Kernel Module      

    - User SDF Library    

    - Test programs
    
    - Documentation
     
    - Postprocessing


- Setting up a new workspace directory for SDFs you will likely want a
  structure::

      kusp/workspaces
          |
	  |----> /sdf_<name>
	             |
                     |----> /docs
		     |----> /include
		     |         |----> /linux
		     |----> /kmods
		     |         |----> /sdf_<name>
		     |----> /libgsched
		     |         |----> /sdf_<name>		       
		     |----> /tests


