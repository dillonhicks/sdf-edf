SHELL:=/bin/bash
BUILDDIR:=./build
PROJECTNAME:=cpu-bound-workloops
ARGS:= --config=../thread_config.xml

all:
	-rm -rf $(BUILDDIR)
	mkdir -p $(BUILDDIR)
	pushd $(BUILDDIR); \
	cmake ..; \
	make

run:
	pushd $(BUILDDIR); \
	./$(PROJECTNAME) $(ARGS)

clean:
	-rm -rf $(BUILDDIR)
	-rm -f *~

