SHELL:=/bin/bash
BUILDDIR:=./build
PROJECTNAME:=edfup1
CPUS=1
DSKICTRLARGS:= --config=$(PROJECTNAME)_$(CPUS)cpu.dski
PPDIR:=$(BUILDDIR)/tmp

.PHONY: all build run clean pp0 pp1 pp2

all: clean build


build:
	mkdir -p $(BUILDDIR); \
	pushd $(BUILDDIR); \
	cmake ..; \
	make


run:
	pushd $(BUILDDIR)/$(CPUS)cpu; \
	dskictrl $(DSKICTRLARGS)


clean:
	-rm -rf $(BUILDDIR)
	-rm -f *~

debug:
	@cd $(BUILDDIR); \
	echo ; \
	echo ; \
	./edfup1 --config=1cpu/edfup1_1cpu.xml -p


pp0:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dsui.bin

pp1:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dski.bin/chan1/*.bin

pp2:
	postprocess -n $(PPDIR)/$(PROJECTNAME)_$(CPUS)cpu.dski.bin/chan2/*.bin

