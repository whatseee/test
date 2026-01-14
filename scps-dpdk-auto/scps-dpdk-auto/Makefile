#
#  Top-level Make file
#
ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overwritten by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

ifneq ($(CONFIG_RTE_EXEC_ENV),"linuxapp")
$(info This application can only operate in a linuxapp environment, \
please change the definition of the RTE_TARGET environment variable)
else

DIRS-y += source apps lib bin
endif

##SHELL=/bin/sh
##BUILDDIRS=source apps FP bin lib

##all clean distclean:: FRC
##	list='$(DIRS)';for dir in $$list; do \
##	  (cd $$dir; pwd; \
##	    eval "make -k $@";); \
##	  done

##status:
##	cvs status -v . | egrep Status | egrep -v "Up-to-date"

##crcchk scpsdiff:: FRC
##	cd FP; pwd; eval "make $@";

##sfp server:: FRC
##	cd source; pwd; eval "make"; \
##	cd ../FP; pwd; eval "make $@";

##scps_ttcp scps_init scps_resp:: FRC
##	cd source; pwd; eval "make"; \
##	cd ../apps; pwd; eval "make $@";

##FRC:

include $(RTE_SDK)/mk/rte.extsubdir.mk

