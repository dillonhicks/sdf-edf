SHELL:=/bin/bash
BUILDDIR:=./build
PROJECTNAME:=edfup0
CPUS=1
DSKICTRLARGS:= --config=$(PROJECTNAME)_$(CPUS)cpu.dski
PPDIR:=$(BUILDDIR)/tmp

all:
	-rm -rf $(BUILDDIR)
	mkdir -p $(BUILDDIR)
	pushd $(BUILDDIR); \
	cmake ..; \
	make


run:
	pushd $(BUILDDIR)/$(CPUS)cpu; \
	dskictrl $(DSKICTRLARGS)


clean:
	-rm -rf $(BUILDDIR)
	-rm -f *~


pp0:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dsui.bin

pp1:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dski.bin/chan1/*.bin

pp2:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dski.bin/chan2/*.bin

