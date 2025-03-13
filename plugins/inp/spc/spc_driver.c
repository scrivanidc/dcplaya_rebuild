/*
 * $Id$
 */

#include "dcplaya/config.h"
#include "extern_def.h"

DCPLAYA_EXTERN_C_START
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
DCPLAYA_EXTERN_C_END

#include "libspc.h"

#include "playa.h"
#include "fifo.h"
#include "inp_driver.h"

#include "exceptions.h"


volatile static int ready; /**< Ready flag : 1 when music is playing */

/** SPC config */
static SPC_Config spc_config = {
  32000, /* frequency (SPC hardware frequency is 32Khz) */
  //16000,
  //44100,
  16,    /* bits per sample */
  2,     /* number of channels */
  1,     /* is interpolation ? */
  1,     /* is echo ? */
};

/** Current SPC info */
static SPC_ID666 spcinfo;

/** PCM buffer */
static int32 *buf;
static int buf_size;
static int buf_cnt;

static void clean_spc_info(void)
{
  memset(&spcinfo, 0, sizeof(spcinfo));
}

static int init(any_driver_t *d)
{
  EXPT_GUARD_BEGIN;

  dbglog(DBG_DEBUG, "sc68 : Init [%s]\n", d->name);
  ready = 0;
  buf = 0;
  buf_size = 0;
  buf_cnt = 0;
  clean_spc_info();


  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return 0;
}

static int stop(void)
{
  if (ready) {
    SPC_close();
  }
  if (buf) {
    free(buf);
  }
  buf = 0;
  buf_size = 0;
  buf_cnt = 0;
  clean_spc_info();
  ready = 0;

  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  EXPT_GUARD_BEGIN;

  stop();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return 0;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  EXPT_GUARD_BEGIN;

  stop();

  buf_size = SPC_init(&spc_config);
  if (buf_size <= 0 || (buf_size&3)) {
    goto error;
  }
  buf = malloc(buf_size);
  if (!buf) {
    goto error;
  }
  buf_size >>= 2;

  dbglog(DBG_DEBUG, "SPC buffer size = %d\n", buf_size);

  if (! SPC_load(fn, &spcinfo)) {
    goto error;
  }

  //playa_info_bps    (info, 0);
  playa_info_desc   (info, "SNES music");
  playa_info_frq    (info, spc_config.sampling_rate);
  playa_info_bits   (info, spc_config.resolution);
  playa_info_stereo (info, spc_config.channels-1);
  playa_info_time   (info, spcinfo.playtime * 1000);
  //  spc_config.is_interpolation = 1;

  buf_cnt = buf_size;
  ready = 1;

  EXPT_GUARD_RETURN 0;

 error:
  stop();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return -1;
}

static int decoder(playa_info_t *info)
{
  int n;

  EXPT_GUARD_BEGIN;

  if (!ready) {
    EXPT_GUARD_RETURN INP_DECODE_ERROR;
  }
        
  if (buf_cnt >= buf_size) {
    SPC_update((char *)buf);
    buf_cnt = 0;
  }

  n = fifo_write( (int *)buf + buf_cnt, buf_size - buf_cnt);
  if (n > 0) {
    buf_cnt += n;
  } else if (n < 0) {
    EXPT_GUARD_RETURN INP_DECODE_ERROR;
  }


  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return -(n>0) & INP_DECODE_CONT;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}

static char * mystrdup(const char *s)
{
  if (!s || !*s) {
    return 0;
  } else {
    return strdup(s);
  }
}

static const char * spc_emul_type(SPC_EmulatorType t) {
  switch(t) {
  case SPC_EMULATOR_ZSNES:
    return "zsnes";
  case SPC_EMULATOR_SNES9X:
    return "snes-9x";
  }
  return "unknown-snes";
}

static int id_info(playa_info_t *info, SPC_ID666 * idinfo)
{
  char tmp[256];

  EXPT_GUARD_BEGIN;

  if ( ! idinfo) {
    idinfo = &spcinfo;
  }
  if (! idinfo->valid) {
    EXPT_GUARD_RETURN -1;
  }

  info->valid   = 0;
  //  info->info    = 0;

  sprintf(tmp, "SNES spc music on %s emulator",
	  spc_emul_type(idinfo->emulator));
  info->info[PLAYA_INFO_FORMAT].s   = mystrdup(tmp);
  // VP : playa_make_time_str is not existing anymore ...
  //  info->info[PLAYA_INFO_TIME].v     = playa_make_time_str(idinfo->playtime * 1000);
 
  playa_info_artist   (info, idinfo->author);
  playa_info_album    (info, idinfo->gametitle);
  //  playa_info_track    (info, 0);
  playa_info_title    (info, idinfo->songname);
  //playa_info_year     (info, 0);
  playa_info_genre    (info, "Video Games");
  //playa_info_comments (info, 0);

  if (idinfo->dumper[0]) {
    sprintf(tmp, "Ripped by %s", idinfo->dumper);
    playa_info_comments (info, tmp);
  }

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return 0;
}

static int info(playa_info_t *info, const char *fname)
{
  EXPT_GUARD_BEGIN;

  if (fname) {
    SPC_ID666 id666;

    SPC_get_id666(fname, &id666);
    EXPT_GUARD_RETURN id_info(info, &id666);
  } else {
    EXPT_GUARD_RETURN id_info(info, 0);
  }

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
}


/* LUA interface */

#include "luashell.h"

static int lua_profile(lua_State * L)
{
  int old = SPC_debugcolor;
  SPC_debugcolor = lua_tonumber(L, 1);
  lua_settop(L, 0);
  lua_pushnumber(L, old);
  vid_border_color(0, 0, 0);
  return 1;
}

static int lua_echo(lua_State * L)
{
  int old = spc_config.is_echo;
  spc_config.is_echo = lua_tonumber(L, 1);
  lua_settop(L, 0);
  lua_pushnumber(L, old);
  vid_border_color(0, 0, 0);
  return 1;
}

static int lua_interpolation(lua_State * L)
{
  int old = spc_config.is_interpolation;
  spc_config.is_interpolation = lua_tonumber(L, 1);
  lua_settop(L, 0);
  lua_pushnumber(L, old);
  vid_border_color(0, 0, 0);
  return 1;
}

static int lua_frequency(lua_State * L)
{
  int nparam = lua_gettop(L);
  int old = spc_config.sampling_rate;
  if (nparam >= 1) {
    float new  = lua_tonumber(L, 1);
    if (new < 8) new = 8;
    if (new > 50) new = 50;
    spc_config.sampling_rate = new*1000;
    playa_info_frq    (info, spc_config.sampling_rate);
  }
  lua_settop(L, 0);
  lua_pushnumber(L, old/1000);
  return 1;
}

static int lua_slowdown(lua_State * L)
{
  int nparam = lua_gettop(L);

  int old1 = SPC_slowdown_cycle_shift;
  int old2 = SPC_slowdown_instructions;
  if (nparam >= 2) {
    SPC_slowdown_cycle_shift = lua_tonumber(L, 1);
    SPC_slowdown_instructions = lua_tonumber(L, 2);
  }
  lua_settop(L, 0);
  lua_pushnumber(L, old1);
  lua_pushnumber(L, old2);
  return 2;
}

static luashell_command_description_t commands[] = {
  {
    "spc_profile", 0, "spc",     /* long name, short name, topic */
    "spc_profile(mode) : set profile mode on or off",  /* usage */
    SHELL_COMMAND_C, lua_profile      /* function */
  },
  {
    "spc_slowdown", 0, 0, /* long name, short name, topic */
    "spc_slowdown([cycle_shift, instructions]) :"
    " set or return SPC_slowdown_cycle_shift"
    " and SPC_slowdown_instructions",               /* usage */
    SHELL_COMMAND_C, lua_slowdown /* function */
  },
  {
    "spc_frequency", "spc_frq", "spc",     /* long name, short name, topic */
    "spc_frequency(frq) : set or return replay frequency in Khz",  /* usage */
    SHELL_COMMAND_C, lua_frequency      /* function */
  },
  {
    "spc_echo", 0, "spc",     /* long name, short name, topic */
    "spc_echo(status) : set or return echo status",  /* usage */
    SHELL_COMMAND_C, lua_echo      /* function */
  },
  {
    "spc_interpolation", 0, "spc",     /* long name, short name, topic */
    "spc_interpolation(status) : set or return interpolation status",  /* usage */
    SHELL_COMMAND_C, lua_echo      /* function */
  },

  /* end of the command list */
  {0},
};


static inp_driver_t spc_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /**< Next driver (see any_driver.h)  */
    INP_DRIVER,           /**< Driver type */      
    0x0100,               /**< Driver version */
    "spc",                /**< Driver name */
    "Vincent Penne, "     /**< Driver authors */
    "Benjamin Gerard, "
    "Gary Henderson, "
    "Jerremy Koot",
    "SNES music player",  /**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
    commands,             /**< Lua shell commands */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  ".spc\0",               /**< EXtension list */

  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(spc_driver)
