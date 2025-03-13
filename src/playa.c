/**
 * @file     playa.c
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    music player threads
 *
 * $Id$
 */

#include <kos.h>

#include "dcplaya/config.h"
#include "sysdebug.h"
#include "playa.h"
#include "inp_driver.h"
#include "sndstream.h"

/* #include "pcm_buffer.h" */
#include "driver_list.h"
#include "file_wrapper.h"
#include "fifo.h"
//#include "fft.h"

#include "priorities.h"

#define PLAYA_THREAD

#define VCOLOR(R,G,B) vid_border_color(R,G,B)

static int current_frq, next_frq;
static int current_stereo;

static inp_driver_t * driver = 0;


int playa_info(playa_info_t * info, const char *fn);
static char * make_default_name(const char *fn);
const char * playa_statusstr(int status);

kthread_t * playa_thread;
static kthread_t * sndstream_thd;
static semaphore_t *playa_haltsem;
static int playavolume = 255;
static int playa_paused;
static volatile int playastatus = PLAYA_STATUS_INIT;
static volatile unsigned int play_samples_start;
static volatile unsigned int play_samples;
static volatile int streamstatus = PLAYA_STATUS_INIT;

static int out_buffer[1<<14];
static int out_samples;
static int out_count;

int playa_force_prio;

int playa_get_frq(void)
{
  return current_frq;
}

void playa_get_buffer(int **b, int *samples, int * counter, int * frq)
{
  *b = (int *)out_buffer;
  *samples = out_samples;
  *counter = out_count;
  *frq = current_frq;
}

static int fade_v, fade_ms;
static const int fade_max = (1<<(16+9))-1;

static void fade(int *d, int n)
{
  int fade_step, ms, v;
  const int signbit = (sizeof(int)<<3)-1;

  if (!current_frq || n <= 0) {
    return;
  }

  ms = fade_ms;
  if (!ms) {
    /* Fade is finished just apply it */
    if (!fade_v) {
      /* Fade-out clears buffer */
      do{
	*d++ = 0;
      } while (--n);
    }
    /* Fade-in does nothing. */
    return;
  }

  v = fade_v;
  fade_step = (1<<(16+14)) / (current_frq * ms >> 5);

  if (fade_step < 0) {
    do {
      int l,r,m,v2;
      l = *d;
      r = l>>16;
      l = (short)l;
      v2 = v>>9;
      *d++ = ((r * v2) & 0xFFFF0000) | ((unsigned int)(l*v2) >> 16);
      v += fade_step;
      m = ~(v>>signbit);
      v &= m;
      fade_step &= m;
    } while (--n);
  } else {
    do {
      const int maxv = fade_max;
      int l,r,m,v2;
      l = *d;
      r = l>>16;
      l = (short)l;
      v2 = v>>9;
      *d++ = ((r * v2) & 0xFFFF0000) | ((unsigned int)(l*v2) >> 16);
      v += fade_step;
      m = (maxv-v) >> signbit;
      v |= m;
      v &= maxv;
      fade_step &= ~m;
    } while (--n);
  }
  fade_v = v;
  if (!fade_step) {
/*     SDDEBUG("Fade %d stop\n", fade_ms); */
    fade_ms = 0;
  }
}

/* Anti click fade out */
static unsigned int fade_out(int *d, int n, int last)
{
  if (n<=0) return last;
  if (!last) {
    do{
      *d++ = 0;
    } while (--n);
  } else {
    do {
      const int f = 0xf00;
      int l = (signed short)last;
      int r = last>>16;
      l = (l * f) >> 12;
      r = (r * f) >> 12;
      *d++ = last = (r << 16) | (l & 0xFFFF);
    } while (--n);
  }
  return last;
}

static void * sndstream_callback(int size)
{
  int last_sample = 0;
  int n;
  //  VCOLOR(255,0,0);


  size >>= 2;
  if (playa_paused && !fade_v) {
    n = 0;
  } else {
#ifdef PLAYA_THREAD

    if (playa_force_prio) {
      playa_thread->prio2 = playa_force_prio;
    } else {
    int used = fifo_used();
    static int old_jiffies;

    /* VP : change priority of the decoder thread with respect to the
       filled ratio of the fifo.
       This priority boosting cannot happen before MINIMUM_TIME_BEFORE_BOOST
       seconds since the fifo was empty (so that we don't get a mini slowdown 
       at the start of a track)
    */
#define MINIMUM_TIME_BEFORE_BOOST (1.0f)
    if (used == 0) {
      old_jiffies = jiffies;
      playa_thread->prio2 = PLAYA_DECODER_THREAD_PRIORITY;
    } else if (jiffies - old_jiffies > MINIMUM_TIME_BEFORE_BOOST*HZ) {

      /* eliminate overflow */
      old_jiffies = jiffies - MINIMUM_TIME_BEFORE_BOOST*HZ;

      /* decoder priority selection */
      int ratio = used * 1000 / fifo_size();
      if (ratio > 40) {
	/* Lowest priority, let's take our time to decode the music :) */
	playa_thread->prio2 = PLAYA_DECODER_THREAD_PRIORITY;
      } else if (ratio > 20) {
	/* Boost the decoder thread ! (mid level) */
	playa_thread->prio2 = PLAYA_DECODER_THREAD_BOOST2_PRIORITY;
      } else {
	/* Boost the decoder thread ! (emergency level) */
	playa_thread->prio2 = PLAYA_DECODER_THREAD_BOOST1_PRIORITY;
      }
    }

    // VP : TEMPORARY TO TEST FFMPEG !!!!
    //playa_thread->prio2 = PLAYA_DECODER_THREAD_BOOST1_PRIORITY;

    }
#endif

    n = fifo_read(out_buffer, size);
  }

  //  VCOLOR(0,0,0);
  if (n < 0) {
    return 0;
  } else {
    int pbs = play_samples_start;

    if (pbs) {
      /* 	  SDDEBUG("Real start in %u samples (%d)\n", pbs, n); */
      pbs -= n;
      if (pbs < 0) {
        /* We reach the new music in the fifo.
	   We had to change stream_parameters */
        if (next_frq != current_frq) {
/*           SDDEBUG("[%s] :  On the fly sampling change : %d->%d\n", */
/* 		  __FUNCTION__, current_frq, next_frq); */
          stream_frq(current_frq = next_frq);
        }
        play_samples = -pbs-n;
/* 	SDDEBUG("New sample count:%u\n", play_samples+n); */
        pbs = 0;
      }
      play_samples_start = pbs;
    }
    play_samples += (unsigned int)n;
    if (n > 0) {
      last_sample = out_buffer[n-1];
    }
    last_sample = fade_out(out_buffer+n, size-n, last_sample);
    fade(out_buffer,size);
  }
  out_samples = size;
  out_count++;
  //  fft_queue(out_buffer, size, current_frq);
  return out_buffer;
}

void sndstream_thread(void *cookie)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);

  thd_current->prio--;
  thd_current->prio2 = PLAYA_SNDSTREAM_THREAD_PRIORITY;

  stream_init(sndstream_callback, 1<<14);
  stream_start(1200, current_frq=44100, playavolume, current_stereo=1);

  // $$$ Aprox sync VBL
  //stream_start(736, current_frq=44100, playavolume, current_stereo=1);
  //  stream_start(256, 44100, playavolume, 1);

  streamstatus = PLAYA_STATUS_PLAYING;
  while (streamstatus == PLAYA_STATUS_PLAYING) {
    int e;

    e = stream_poll();
    //thd_pass();

    //if (e <= 0)
      usleep(1000000/60/4);

#if 0
    if (!e) {
      /* nothing happened */
      //thd_pass();
    } else if (e<0) {
      /* callback has return a null pointer */
    } else {
      /* audio data has been sent to hardware*/
    }
#endif
  }

  /* This is done by the stop function :
     streamstatus = PLAYA_STATUS_STOPPING;
  */

  stream_stop();
  stream_shutdown();

  streamstatus = PLAYA_STATUS_ZOMBIE;
  SDDEBUG("<< %s()\n", __FUNCTION__);
}

static void real_playa_update(void)
{
  switch (playastatus) {

  case PLAYA_STATUS_INIT:
    playastatus = PLAYA_STATUS_READY;
    break;

  case PLAYA_STATUS_READY:
    SDDEBUG("%s() : waiting on semaphore\n", __FUNCTION__);
#ifdef PLAYA_THREAD
    sem_wait(playa_haltsem);
#endif
    SDDEBUG("%s() : released from semaphore\n", __FUNCTION__);
    break;

  case PLAYA_STATUS_STARTING:
    playastatus = PLAYA_STATUS_PLAYING;
    break;

  case PLAYA_STATUS_REINIT:
    /* Re-initialize streaming driver */
    /* ??? */
    playastatus = PLAYA_STATUS_READY;
    break;

  case PLAYA_STATUS_PLAYING:
    {
      int status;
      playa_info_t info;
      //      VCOLOR(255,255,0);
      if (!driver) {
	SDERROR("No driver !\n");
	status = INP_DECODE_ERROR;
      } else if (!fifo_free()) {
/* 	SDDEBUG("Fifo is full ... pass ...\n"); */
	status = 0;
      } else {
	memset(&info,0,sizeof(info));
	status = driver->decode(&info);
      }
      //      VCOLOR(0,0,0);

      if (status & INP_DECODE_END) {
	if (driver && status == INP_DECODE_ERROR) {
	  SDERROR("Driver error\n");
	}
	/* 		SDDEBUG("STOOOOOOOOP\n"); */
	playastatus = PLAYA_STATUS_STOPPING;
	break;
      }

      if (status & INP_DECODE_INFO) {
/* 	SDDEBUG("Driver change INFO : %x\n", info.update_mask); */
	playa_info_update(&info);

	/* VP : added that in case frequency changes */
	/* Set sampling rate for next music */
	if (info.info[PLAYA_INFO_FRQ].v &&
	    next_frq != info.info[PLAYA_INFO_FRQ].v) {
	  stream_frq(next_frq = current_frq = info.info[PLAYA_INFO_FRQ].v);
	  SDDEBUG("Set stream frq := [%d]\n",current_frq);
	}
      }

      if (! (status & INP_DECODE_CONT)) {
	//thd_pass();
	usleep(1000000/60/5);
      }

    } break;

  case PLAYA_STATUS_QUIT:
    break;

  case PLAYA_STATUS_STOPPING:
    {
      if (driver) {
	driver->stop();
      }
      playa_info_clean();
      playastatus = PLAYA_STATUS_READY;
    } break;
  }
}

#ifdef PLAYA_THREAD
static
#endif
void playa_update(void)
{
#ifdef PLAYA_THREAD
  thd_pass();
#else
  real_playa_update();
#endif
}

static void wait_playastatus(const int status)
{
  /* VP : hum ... that would be better to use a real timer */
  const int retimeout = 20000;
  int timeout = 1;

  while (playastatus != status) {
    if (!--timeout) {
      SDDEBUG("Player [%s] waiting [%s]\n",
	      playa_statusstr(playastatus),
	      playa_statusstr(status));
      timeout = retimeout;
    }
    playa_update();
  }
}


#ifdef PLAYA_THREAD
void playadecoder_thread(void *blagh)
{
  int oldstatus = playastatus;

  playa_thread = thd_current;

  SDDEBUG(">> %s()\n", __FUNCTION__);

  /* Main command loop */
  while (playastatus != PLAYA_STATUS_QUIT) {
    if (playastatus != oldstatus) {
      SDDEBUG("%s() : STATUS [%s] -> [%s]\n", __FUNCTION__,
	      playa_statusstr(oldstatus), playa_statusstr(playastatus));
      oldstatus = playastatus;
    }
    real_playa_update();
  }

  /* Done: clean up */
  playastatus = PLAYA_STATUS_ZOMBIE;
  SDDEBUG("<< %s()\n", __FUNCTION__);
}
#endif

int playa_init()
{
  int e = 0;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

/*   pcm_buffer_init(0,0); */
  fade_v = 0;
  fade_ms = 0;
  playa_paused = 0;

  playa_info_init();
  fifo_init(1024 * 256);
  //  fft_init(4);

  playa_haltsem = sem_create(0);

#if 0
  /* VP : use a timer interruption instead */
  stream_init(sndstream_callback, 1<<14);
  stream_start(1200, current_frq=44100, playavolume, current_stereo=1);

  streamstatus = PLAYA_STATUS_PLAYING;
  SDDEBUG("READY soundstream interrupt\n");
  
#else
  SDDEBUG("Create soundstream thread\n");
  streamstatus = PLAYA_STATUS_INIT;
  sndstream_thd = thd_create(sndstream_thread, 0);
  if (sndstream_thd) {
    thd_set_label(sndstream_thd, "Sound-stream-thd");
  }

  SDDEBUG("Waiting soundstream thread\n");
  while (streamstatus != PLAYA_STATUS_PLAYING)
    thd_pass();
  SDDEBUG("READY soundstream thread\n");
#endif

  SDDEBUG("Create PLAYA decoder thread\n");
  playastatus = PLAYA_STATUS_INIT;
#ifdef PLAYA_THREAD
  {
    int old = thd_default_stack_size;
    thd_default_stack_size = 64*1024;
    playa_thread = thd_create(playadecoder_thread, 0);
    if (playa_thread) {
      thd_set_label(playa_thread, "Playa-thd");
    }
    thd_default_stack_size = old;
  }
#endif
  SDDEBUG("Waiting PLAYA decoder thread\n");
  wait_playastatus(PLAYA_STATUS_READY);
  SDDEBUG("READY PLAYA decoder thread\n");

  SDUNINDENT;
  SDDEBUG("<< %s() := [%d]\n",__FUNCTION__, e);
  return e;
}

int playa_shutdown()
{
  int e = 0;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  playa_stop(1);

  /* PLAYA decoder */
  SDDEBUG("Decoder stream\n");
  playastatus = PLAYA_STATUS_QUIT;
  sem_signal(playa_haltsem);
  wait_playastatus(PLAYA_STATUS_ZOMBIE);
  sem_destroy(playa_haltsem);

  /* Sound Stream */
  SDDEBUG("Shutdown sound stream\n");
  if (streamstatus == PLAYA_STATUS_PLAYING) {
    SDDEBUG("Waiting sound stream stop\n");
    streamstatus = PLAYA_STATUS_STOPPING;
    while (streamstatus != PLAYA_STATUS_ZOMBIE)
      ;
    SDDEBUG("STOPPED\n");
  }

  /* $$$ May be it is not the better place to do that :
     driver was initialized in an other file */
  {
    inp_driver_t *d;
    for (d=(inp_driver_t *)inp_drivers.drivers;
	 d;
	 d=(inp_driver_t *)d->common.nxt) {
      e |= d->common.shutdown(&d->common);
    }
  }

  playa_info_shutdown();

  SDUNINDENT;
  SDDEBUG("<< %s() := [%d]\n",__FUNCTION__, e);

  return e;
}

int playa_isplaying() {
  return playastatus == PLAYA_STATUS_PLAYING;
}

int playa_status() {
  return playastatus;
}

const char * playa_statusstr(int status) {
  static const char * statusstr[] =
    {
      "INIT",
      "READY",
      "STARTING",
      "PLAYING",
      "STOPPING",
      "QUIT",
      "ZOMBIE",
      "REINIT",
    };
  return (status < 0 || status > 7) ? "???" : statusstr[status];
}


static int StrLen(const char *s)
{
  return s ? strlen(s) : 0;
}


int playa_info(playa_info_t * info, const char *fn)
{
  int err;

  memset(info,0,sizeof(*info));
  err = driver ? driver->info(info, fn) : -1;
  if (err < 0) {
    /* $$$ ben: Safety net to be sure the driver does not trash it ... 
     * Memory leaks are possible anyway ... 
     */
    memset(info,0,sizeof(*info));
  }
  return err;
}

int playa_stop(int flush)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);
  
  /* Already stopped */
  if (playastatus == PLAYA_STATUS_READY) {
    goto end;
  }

  /* Ask thread to stop */
  playastatus = PLAYA_STATUS_STOPPING;
  wait_playastatus(PLAYA_STATUS_READY);

  /* If flush or paused, clean PCM FIFO */
  if (flush || playa_paused) {
    SDDEBUG("[%s] : Fush FIFO\n", __FUNCTION__);
    fifo_start();
    play_samples = play_samples_start = 0;
    fade_ms = 0;
    fade_v  = 0;
  }

  driver_dereference(&driver->common);
  driver = 0;

 end:

  SDUNINDENT;
  SDDEBUG("<< %s() := [0]\n", __FUNCTION__);

  return 0;
}


int playa_start(const char *fn, int track, int immediat) {
  int e = -1;
  playa_info_t info;
  inp_driver_t *d = 0;

  SDDEBUG(">> %s('%s',%d, %d)\n",__FUNCTION__, fn, track, immediat);
  SDINDENT;

  memset(&info, 0, sizeof(info));

  if (!fn) {
    SDERROR("null filename\n");
    goto error;
  }

  /* $$$ Try to find a driver for this file. A quick glance at file extension
     will be suffisant right now. Later the driver should support an is_mine()
     function. */
  /* This create a reference on the driver. */
  d = inp_driver_list_search_by_extension(fn);

  /* $$$ Here, the previous play is not stopped. May be songmenu.c expects
     it is !!! */
  // Can't start again if already playing
  immediat |= playa_paused;
  playa_stop(immediat);

  //$$$
  if (!immediat) {
    int pbs = fifo_used();
    if (pbs < 0) {
      SDWARNING("fifo used : %d !!!\n", pbs);
      pbs = 0;
    }
    SDWARNING("Real start in %u samples!!!\n", pbs);
    play_samples_start = pbs;
  } else {
    /* */
    play_samples = play_samples_start = 0;
    fade_ms = 128;
    fade_v = 0;
  }
  
  /* Start playa, get decoder info */

  if (d)
    e = d->start(fn, track, &info);
  else {
    /* VP : added support for multi driver */
    driver_list_lock(&inp_drivers);

    for (d=(inp_driver_t *)inp_drivers.drivers;
	 d;
	 d=(inp_driver_t *)d->common.nxt) {

      if (d->extensions[0] == '*') {
	e = d->start(fn, track, &info);
	if (!e) {
	  driver_reference(&d->common);
	  break;
	}
      }
    }

    driver_list_unlock(&inp_drivers);
  }

  if (!d) {
    SDWARNING("No driver for this file !\n");
    goto error;
  } else {
    SDDEBUG("Driver [%s] found\n", d->common.name);
  }


  if (e) {
    SDERROR("Driver [%s] error := [%d]\n", d->common.name, e);
    goto error;
  }


  // $$$ ben: Validate all fields ? Not sure this is really wise.
  // Yes. it is ok like that, since there is a PLAYA_INFO_MUSIC bit
  // that tell music as change, then all fiels must be updated 
  info.update_mask = (1 << PLAYA_INFO_SIZE) - 1;
  if (!info.info[PLAYA_INFO_TITLE].s) {
    // $$$ ben: direct access ! Not very clean.
    info.info[PLAYA_INFO_TITLE].s = make_default_name(fn);
  }
  playa_info_update(&info);

  /* Set sampling rate for next music */
  if (play_samples_start > 0) {
    next_frq = info.info[PLAYA_INFO_FRQ].v;
  } else {
    stream_frq(next_frq = current_frq = info.info[PLAYA_INFO_FRQ].v);
    SDDEBUG("Set stream frq := [%d]\n",current_frq);
  }

  /* Wait for player thread to be ready */
  wait_playastatus(PLAYA_STATUS_READY);

  /* $$$ Already done by playa_stop() ... It is a safety net. */
  driver_dereference(&driver->common);
  driver_reference(&d->common);
  driver=d;

  /* Tell it to start */
  //  playastatus = PLAYA_STATUS_STARTING;
  playastatus = PLAYA_STATUS_PLAYING;
  sem_signal(playa_haltsem);
  //  wait_playastatus(PLAYA_STATUS_STARTING);
  /*   playa_info_dump(&info); */

 error:
  driver_dereference(&d->common);
  SDUNINDENT;
  SDDEBUG("<< %s([%s],%d,%d) := [%d] \n",__FUNCTION__, fn, track, immediat, e);

  return e;
}

int playa_volume(int volume) {
  int old = playavolume;
  if (volume >= 0) stream_volume(playavolume = volume);
  return old;
}

int playa_ispaused(void)
{
  return playa_paused;
}

int playa_pause(int v)
{
  int old = playa_paused;
  playa_paused = !!v;
  if (playa_paused) {
    playa_fade(-1024);
  } else {
    playa_fade(1024);
  }
  /*   if (playa_paused != old) { */
  /* 	SDDEBUG("%s playa\n", playa_paused ? "Pause" : "Resume"); */
  /*   } */
  return old;
}

int playa_fade(int ms)
{
  int old = fade_ms;
  if (ms) {
    fade_ms = ms;
    SDDEBUG("Start fade [v:%d ms:%d]\n", fade_v, fade_ms);
  }
  return old;
}

unsigned int playa_playtime()
{
  return ((long long)play_samples << 10) / current_frq;
}

static void remove_extension(char *n)
{
  int len;
  char * s;

  s = strrchr(n,'.');
  if (!s) return;

  len = strlen(s);
  if (len <= 5 && s != n) {
    *s = 0;
  }
}

static char * make_default_name(const char *fn)
{
  const char *s , *e;
  char * name;
  int len;

  /* Remove path */
  s = strrchr(fn,'/');
  if (!s) {
    s = fn;
  } else {
    ++s;
  }

  /* Remove extension */
  e = strrchr(s, '.');
  if (e && !stricmp(e,".gz")) {
    const char *e2 = strrchr(s, '.');
    e = e2 ? e2 : e;
  }

  len = e ? (e-s) : strlen(s);
  name = malloc(len+1);
  if (name) {
    int i;
    for (i=0; i<len; ++i) {
      int c = s[i];
      if (c == '_') c = ' ';
      name[i] = c;
    }
    name[i]= 0;
  }
  return name;
}
