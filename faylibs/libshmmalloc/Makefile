#
#  Makefile for both static and dynamic libraries
#

#
# Installation directory
#
INSTDIR   = /usr

#
# Search directories
#
INCLUDE   = -I./

#
# Library name
#
PRO_NAME  = shmmalloc

#
# Source Files
#
HEADERS   = ${PRO_NAME}.h
SOURCES   = ${PRO_NAME}.c
OBJECTS   = ${PRO_NAME}.o

#
# Main targets
#
STATICLIB  = lib${PRO_NAME}.a
DYNAMICLIB = lib${PRO_NAME}.so
#TARGETS    = ${STATICLIB}
TARGETS    = ${DYNAMICLIB}
TEST_PROC  = test_shm_malloc

#
# C compiler and flags
#
CC        = gcc
CFLAGS    = -g -Wall ${INCLUDE}
# for x86_64 system
CFLAGS_EX = -fPIC -errchk=longptr64
LDFLAGS   = -shared
LINKLIBS  = 


# How to make an object file
%.o: %.c %.h
#	@echo "=*=  making object $@ =*="
	@${CC} ${CFLAGS} ${CFLAGS_EX} -c $< -o $@


all: ${TARGETS}

${STATICLIB}: ${OBJECTS}
	@echo "=*= making static library $@ =*="
	@ar rs $@ ${OBJECTS}
	@ranlib $@

${DYNAMICLIB}: ${OBJECTS}
	@echo "=*= making dynamic library $@ =*="
	${CC} ${LDFLAGS} $< ${LINKLIBS} -o $@

install: ${DYNAMICLIB} #${STATICLIB}
#	cp ${STATICLIB} ${INSTDIR}/lib/
	cp ${DYNAMICLIB} ${INSTDIR}/lib/
	cp ${HEADERS} ${INSTDIR}/include/

deinstall:
#	rm -f ${INSTDIR}/lib/${STATICLIB}
	rm -f ${INSTDIR}/lib/${DYNAMICLIB}
	rm -f ${INSTDIR}/include/${HEADERS}

test:
	${CC} ${CFLAGS} ${TEST_PROC}.c -o ${TEST_PROC} -L. -l${PRO_NAME}

clean:
	@rm -f ${TARGETS} ${TEST_PROC} *.o *~
