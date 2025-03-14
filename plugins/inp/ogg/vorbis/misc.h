/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: miscellaneous prototypes
 last mod: $Id$

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_
#include "vorbis/codec.h"
#include "kos_fake_func.h"
#include <malloc.h>

extern void *_vorbis_block_alloc(vorbis_block *vb,long bytes);
extern void _vorbis_block_ripcord(vorbis_block *vb);
extern void _analysis_output(char *base,int i,float *v,int n,int bark,int dB);

#ifdef DEBUG_LEAKS
extern void *_VDBG_malloc(void *ptr,long bytes,char *file,long line); 
extern void _VDBG_free(void *ptr,char *file,long line); 

#ifndef MISC_C 
#undef _ogg_malloc
#undef _ogg_calloc
#undef _ogg_realloc
#undef _ogg_free

#define _ogg_malloc(x) _VDBG__ogg_malloc(NULL,(x),__FILE__,__LINE__)
#define _ogg_calloc(x,y) _VDBG__ogg_malloc(NULL,(x)*(y),__FILE__,__LINE__)
#define _ogg_realloc(x,y) _VDBG__ogg_malloc((x),(y),__FILE__,__LINE__)
#define _ogg_free(x) _VDBG__ogg_free((x),__FILE__,__LINE__)
#endif
#endif

#endif

/* Following lines (for Dreamcast compatibility) added by
 * Thorsten Titze <thorsten.titze@hangar-eleven.de>
 */

#define alloca __builtin_alloca 
//char strdup(const char *);
//void qsort(void *, size_t, size_t, int (*) (const void *, const void *));
