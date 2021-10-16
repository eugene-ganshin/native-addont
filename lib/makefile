###############################################################################
## makefile: ltoapi make file                                                ##
###############################################################################

###############################################################################
# Makefile to make the ltoapi                                                 #
#                                                                             #
# Usage:                                                                      #
#                                                                             #
# To make the ltoapi test program:                                            #
# make testapi                                                                #
#                                                                             #
###############################################################################

#CFLAGS = -Xa -v -O
#CFLAGS = -ansi -pedantic
CFLAGS = -g

#SYSLIB = -lm -lmproc -lsocket -lnsl
SYSLIB = -lsocket -lnsl

###############################################################################
# versions:

# name of machine where API server is running
API_TEST_SERVER  = argentum

# API release version string
API_TEST_VERSION = 1.02a


###############################################################################
# LTCOMMONxxxx are objects that are common to client
#   CLT=client
LTCOMMONCLT=ltCmnC.o ltMsgC.o

# LTCLIENTx are client-only objects
LTCLIENTt=ltClntUt.o

# flags for building client
CLTFLAGS = -DLT_LOG_FILE -DLT_RESET -DLT_ENCRYPT


testapi: testapi.c ltoapi.h $(LTCLIENTt) $(LTCOMMONCLT)
	@ echo
	@ echo 'Building LPI API client test application for test'
	@ echo
	cc -o testapi -DAPI_SERVER=\"$(API_TEST_SERVER)\" $(CLTFLAGS) \
		$(CFLAGS) testapi.c $(LTCLIENTt) $(LTCOMMONCLT) $(SYSLIB)
#	rm testapi.o

###############################################################################

ltCmnC.o: ltCmn.c ltint.h ltoapi.h
	cc -c $(CLTFLAGS) $(CFLAGS) ltCmn.c
	mv ltCmn.o ltCmnC.o

ltMsgC.o: ltMsg.c ltint.h ltoapi.h
	cc -c $(CLTFLAGS) $(CFLAGS) ltMsg.c
	mv ltMsg.o ltMsgC.o

ltClntUt.o: ltClntU.c ltint.h ltoapi.h
	cc -c -DAPI_VERSION=\"$(API_TEST_VERSION)\" $(CLTFLAGS) $(CFLAGS) \
		ltClntU.c
	mv ltClntU.o ltClntUt.o

############################################################### end makefile ##
