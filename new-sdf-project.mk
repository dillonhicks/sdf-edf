SHELL:=/bin/bash
SDFNAME:=dummy

PROJECTDIRS:= docs \
	      include/linux \
	      kmods/sdf_$(SDFNAME) \
	      libgsched/sdf_$(SDFNAME) \
	      tests

.PHONY: all new

all:
	@echo "Error: Use $ make -f sdf-project.mk new SDFNAME=<myname>"
	exit 1;

new:
	for sdfdir in $(PROJECTDIRS); do \
		mkdir -p sdf_$(SDFNAME)/$$sdfdir; \
	done; 
