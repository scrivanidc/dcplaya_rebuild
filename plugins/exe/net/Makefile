#
# Make exe plugins for net
#
# (C) COPYRIGHT 2002 Vincent Penne
#
# $Id$
#

LEZ=yes

KOS_LOCAL_INCS += -I. -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

SUBDIRS = bba lan

include ../../../Makefile.inc

LWIPDIR = $(KOS_BASE)/../kos-ports/lwip/lwip/src
ARCHDIR = $(LWIPDIR)/../../kos

KOS_LOCAL_CFLAGS += -DDCPLAYA=1 -DIPv4 -DLWIP_DEBUG
KOS_INCS +=  \
	-DIPv4 \
	-I$(LWIPDIR)/include -I$(ARCHDIR)/include \
	-I$(LWIPDIR)/include/ipv4 

KOS_LOCAL_INCS += -I../../../include -I../../../libs -I.

LEF_EXTRA += -L$(KOS_BASE)/../kos-ports/lib/dreamcast -llwip4
