# Makefile for zlib
# Copyright (C) 1995-2002 Jean-loup Gailly.
# For conditions of distribution and use, see copyright notice in zlib.h 
#
# dcplaya version by Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id$

LIBRARY=yes
KOS_LOCAL_INCS= -I. -I../../include
KOS_LOCAL_CFLAGS = -DSTDC -DNO_ERRNO_H
OBJS = \
 adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o\
 zutil.o inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o\
 errno.o

include ../../Makefile.inc
