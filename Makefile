CC = gcc
LDFLAGS = 

.PHONY: all scoff clean scoffclean

all: scoff

scoff:
	$(MAKE) -C src

scoffclean:
	$(MAKE) -C src clean

clean: scoffclean
