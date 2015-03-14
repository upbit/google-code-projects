#
# Makefile for both static and dynamic libraries
#

#
# Installation directory
#
INSTDIR   = /usr/local

#
# Search directories
#
INCLUDE   = -I./

#
# Library name
#
PRO_NAME  = shmdbuf

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
TARGETS    = ${STATICLIB}
#TARGETS    = ${DYNAMICLIB}

#
# C compiler and flags
#
CC        = gcc
#CFLAGS    = -g -Wall ${INCLUDE}
CFLAGS    = -DNO_DEBUG -Wall ${INCLUDE}
LDFLAGS   = -shared
LINKLIBS  = -lrt -lpthread


# How to make an object file
%.o: %.c %.h
#	@echo "=*=  making object $@ =*="
	${CC} ${CFLAGS} -c $< -o $@


all: ${TARGETS}

${STATICLIB}: ${OBJECTS}
	@echo "=*= making static library $@ =*="
	ar rs $@ ${OBJECTS}
	ranlib $@

${DYNAMICLIB}: ${OBJECTS}
	@echo "=*= making dynamic library $@ =*="
	${CC} ${LDFLAGS} $< ${LINKLIBS} -o $@

install: ${STATICLIB} #${DYNAMICLIB}
	cp ${STATICLIB} ${INSTDIR}/lib/
#	cp ${DYNAMICLIB}  ${INSTDIR}/lib/
	cp ${HEADERS} ${INSTDIR}/include/

deinstall:
	rm -f ${INSTDIR}/lib/${STATICLIB}
	rm -f ${INSTDIR}/lib/${DYNAMICLIB}
	rm -f ${INSTDIR}/include/${HEADERS}

clean:
	@rm -f ${TARGETS} *.o *~
