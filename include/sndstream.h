/*
 * sndstream.h - SH-4 support routines for SPU streaming sound driver
 *
 * (C) 2000 Dan Potter
 *
 * $Id$
 */

#ifndef __SNDSTREAM_H
#define __SNDSTREAM_H

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/* Load sample data from SH-4 ram into SPU ram (auto-allocate RAM) */
uint32 stream_load_sample(const uint16 *src, uint32 len);

/* Dump all loaded sample data */
void stream_dump_samples();

/* Set "get data" callback */
void stream_set_callback(void *(*func)(int));

/* Prefill buffers -- do this before calling start() */
void stream_prefill();

/* Initialize stream system */
int stream_init(void* (*callback)(int), int samples);

/* Shut everything down and free mem */
void stream_shutdown();

/* Start streaming */
void stream_start(int samples, uint32 freq, int vol, int stereo);

/* Stop streaming */
void stream_stop();

/* Poll streamer to load more data if neccessary; zero if ok, -1 if the
   poll function returns NULL. */
int stream_poll();

/* Start a sound sample on the given channel */
void stream_play_effect(int chn, uint32 src, uint32 freq, uint32 len, uint32 vol, uint32 pan);

/* Stop a sound sample on the given channel */
void stream_stop_effect(int chn);

/* Set the volume on the streaming channels */
void stream_volume(int vol);

/* Set the frequency of thew streaming channel */
void stream_frq(int frq);

/* 0:mono 1:stereo 2:invert-stereo */
void stream_stereo(int stereo);

DCPLAYA_EXTERN_C_END

#endif	/* __SNDSTREAM_H */

