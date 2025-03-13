/**
 * @ingroup    dcplaya
 * @file       dynshell.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @author     benjamin gerard <ben@sashipa.com>
 * @date       2002/11/09
 * @brief      Dynamic LUA shell
 *
 * @version    $Id$
 */

#include "dcplaya/config.h"

#include <stdio.h>
#include <kos.h>

#include "dc/ta.h"

#include "sysdebug.h"

#include "filetype.h"

#include "lua.h"
#include "luadebug.h"
#include "lualib.h"

#include "console.h"
#include "shell.h"
#include "luashell.h"

#include "file_utils.h"
#include "filename.h"
#include "plugin.h"
#include "option.h"
#include "dcar.h"
#include "gzip.h"
#include "playa.h"
#include "vmu_file.h"
#include "fs_ramdisk.h"
#include "draw/texture.h"
#include "translator/translator.h"
#include "translator/SHAtranslator/SHAtranslatorBlitter.h"

#include "exceptions.h"

#include "ds.h"

#ifndef NOTYET
#undef DEBUG
#endif

lua_State * shell_lua_state;

typedef void (* shutdown_func_t)();

static shell_command_func_t old_command_func;

static int song_tag;

static const char home[256];
static const char initfile[256];

static float frame_to_second(unsigned int frames) {
  return frames * (1.0f/60.0f);
}

#define lua_assert(L, test) if (!(test)) lua_assert_func(L, #test); else
static void lua_assert_func(lua_State * L, const char * msg)
{
  lua_error(L, msg);
}

#define CR_SIZE 32
static spinlock_t cr_inmutex;
static spinlock_t cr_outmutex;
static int cr_in;
static int cr_out;
static char * cr[CR_SIZE];

static int lua_get_shell_command(lua_State * L)
{
  int result = 0;

  spinlock_lock(&cr_outmutex);
  if ( cr_in != cr_out ) {
    lua_pushstring(L, cr[cr_out]);
    free(cr[cr_out]);
    cr[cr_out] = 0;
    cr_out = (cr_out+1)&(CR_SIZE-1);

    result = 1;
  }
  spinlock_unlock(&cr_outmutex);

  return result;
}

//static int dynshell_command(const char * fmt, ...)
static int dynshell_command(const char * com)
{
  static int inner; // replace this by a mutex later
  //char com[1024];
  //char com[10*1024];
  //char com[256];
  //char toto[10*1024];
  int result = -1;

  if (inner) {
/*     va_list args; */
/*     va_start(args, fmt); */
/*     vsprintf(com, fmt, args); */
/*     va_end(args); */
    printf("QUEUE COMMAND <%s>\n", com);

    spinlock_lock(&cr_inmutex);
    if ( ((cr_in + 1)&(CR_SIZE-1)) != cr_out ) {
      cr[cr_in] = strdup(com);
      cr_in = (cr_in+1)&(CR_SIZE-1);
      result = 0;
    }
    spinlock_unlock(&cr_inmutex);

    return result;
  }

  inner = 1;
  EXPT_GUARD_BEGIN;

  if (*com) {
/*   if (*fmt) { */
/*     va_list args; */
/*     va_start(args, fmt); */
/*     vsprintf(com, fmt, args); */
/*     va_end(args); */
      
    //result = lua_dostring(shell_lua_state, com);
    lua_getglobal(shell_lua_state, "doshellcommand");
    lua_pushstring(shell_lua_state, com);
    lua_call(shell_lua_state, 1, 0);
    result = 0;
  } else
    result = 0;

  EXPT_GUARD_CATCH;

  shell_showconsole(); /* make sure console is visible */

  /*  printf("CATCHING EXCEPTION IN SHELL !\n");
      irq_dump_regs(0, 0); */

  {
    lua_Debug ar;
    lua_State * L = shell_lua_state;
    int n = 0;
    while (lua_getstack(L, n, &ar)) {
      lua_getinfo (L, "lnS", &ar);
      printf("[%d] "
	     "currentline = %d, "
	     "name = %s, "
	     "namewhat = %s, "
	     "nups = %d, "
	     "linedefined = %d, "
	     "what = %s, "
	     "source = %s, "
	     "short_src = %s\n",
	     n,
	     ar.currentline,
	     ar.name,
	     ar.namewhat,
	     ar.nups,
	     ar.linedefined,
	     ar.what,
	     ar.source,
	     ar.short_src);
      n++;
      break; /* crash if going too far ... */
    }
  }


  EXPT_GUARD_END;

  inner = 0;
  return result;
  
}

static int fdynshell_command(const char * fmt, ...)
{
  char com[2048];
  //char com[256];

  //printf("fdynshell '%s'\n", fmt);

  va_list args;
  va_start(args, fmt);
  vsprintf(com, fmt, args);
  va_end(args);

  return dynshell_command(com);
}

/* dynamic structure lua support */
#define lua_push_entry(L, desc, data, entry) lua_push_entry_func((L), &desc##_DSD, (data), (entry) )

static int lua_push_entry_func(lua_State * L, ds_structure_t * desc, void * data, const char * entryname)
{
  ds_entry_t * entry;

  entry = ds_find_entry_func(desc, entryname);

  if (entry) {
    switch (entry->type) {
    case DS_TYPE_INT:
      lua_pushnumber(L, * (int *) ds_data(data, entry));
      return 1;
    case DS_TYPE_STRING:
      lua_pushstring(L, * (char * *) ds_data(data, entry));
      return 1;
    case DS_TYPE_LUACFUNCTION:
      lua_pushcfunction(L, * (lua_CFunction *) ds_data(data, entry));
      return 1;
    }
  }

  return 0;
}



/* driver type */

#include "driver_list.h"

static
#include "luashell_ds.h"

static int shellcd_tag;

static int lua_shellcd_gettable(lua_State * L)
{
  const char * field;
  luashell_command_description_t * cd;

  cd = lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  if (cd->type == SHELL_COMMAND_LUA && !strcmp(field, "function")) {
    /* cast to lua function */

    // need to read the doc ...
    
    return 0;
  }

  lua_assert(L, field);

  return lua_push_entry(L, luashell_command_description_t, cd, field);
}

static int lua_shellcd_settable(lua_State * L)
{
  const char * field;
  luashell_command_description_t * cd;

  cd = lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  if (!strcmp(field, "registered")) {
    cd->registered = lua_tonumber(L, 3);
  }

  return 0;
}



static
#include "any_driver_ds.h"

static int driver_tag;
static int driverlist_tag;


static int lua_driverlist_gettable(lua_State * L)
{
  driver_list_t * table = lua_touserdata(L, 1);
  any_driver_t * driver = NULL;
  any_driver_t * * pdriver = NULL;
  if (lua_isnumber(L, 2)) {
    // access by number
    int n = lua_tonumber(L, 2);
    lua_assert(L, n>=1 && n<=table->n);
    driver = driver_list_index(table, n-1);
  } else {
    // access by name
    const char * name = lua_tostring(L, 2);

    lua_assert(L, name);
    if (!strcmp(name, "n")) {
      /* asked for number of entry */
      lua_settop(L, 0);
      lua_pushnumber(L, table->n);
      return 1;
    } else {
      driver = driver_list_search(table, name);
    }
  }

  lua_assert(L, driver);

  pdriver = (any_driver_t * *) malloc(sizeof(any_driver_t * *));
  lua_assert(L, pdriver);
  *pdriver = driver;
  
  lua_settop(L, 0);
  lua_pushusertag(L, pdriver, driver_tag);

  return 1;
}

static int lua_driver_is_same(lua_State * L)
{
  any_driver_t * * pdriver1;
  any_driver_t * * pdriver2;
  int cmp;

  pdriver1 = lua_touserdata(L, 1);
  pdriver2 = lua_touserdata(L, 2);

  cmp = (pdriver1 && pdriver2 && *pdriver1 == *pdriver2);

  lua_settop(L, 0);
  if (cmp) {
    lua_pushnumber(L, 1);
    return 1;
  } else
    return 0;
}

static int lua_driver_gc(lua_State * L)
{
  any_driver_t * * pdriver;
  pdriver = lua_touserdata(L, 1);
  driver_dereference(*pdriver);
  free(pdriver);

  return 0;
}

static int lua_driver_gettable(lua_State * L)
{
  const char * field;
  any_driver_t * driver;

  driver = *(any_driver_t * *) lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  lua_assert(L, driver);
  lua_assert(L, field);

  if (driver->luacommands && !strcmp(field, "luacommands")) {
    /* special case for luacommands field : 
       return table with all lua commands */
    int i;

    lua_settop(L, 0);
    lua_newtable(L);

    for (i=0; driver->luacommands[i].name; i++) {
      lua_pushnumber(L, i+1);
      lua_pushusertag(L, driver->luacommands+i, shellcd_tag);
      lua_settable(L, 1);
    }
    
    return 1;
  }

  return lua_push_entry(L, any_driver_t, driver, field);
}

static int lua_driver_settable(lua_State * L)
{
  return 0;
}

static void register_driver_type(lua_State * L)
{

  shellcd_tag = lua_newtag(L);
  lua_pushcfunction(L, lua_shellcd_gettable);
  lua_settagmethod(L, shellcd_tag, "gettable");

  lua_pushcfunction(L, lua_shellcd_settable);
  lua_settagmethod(L, shellcd_tag, "settable");


  /* driver_list */
  driverlist_tag = lua_newtag(L);
  lua_pushnumber(L,driverlist_tag);
  lua_setglobal(L,"driverlist_tag");

  lua_pushcfunction(L, lua_driverlist_gettable);
  lua_settagmethod(L, driverlist_tag, "gettable");

  /* driver */
  driver_tag = lua_newtag(L);
  lua_pushnumber(L,driver_tag);
  lua_setglobal(L,"driver_tag");

  lua_pushcfunction(L, lua_driver_gc);
  lua_settagmethod(L, driver_tag, "gc");

  lua_pushcfunction(L, lua_driver_gettable);
  lua_settagmethod(L, driver_tag, "gettable");

  lua_pushcfunction(L, lua_driver_settable);
  lua_settagmethod(L, driver_tag, "settable");
}


////////////////
// LUA commands
////////////////
static int lua_malloc_stats(lua_State * L)
{
  malloc_stats();
  texture_memstats();
  return 0; // 0 return values
}

/* Return 2 lists
   1st has been filled with directory entries,
   2nd has been filled with file entries.
   Each list has been sorted according to sortdir function.
*/
static int push_dir_as_2_tables(lua_State * L, fu_dirent_t * dir, int count,
				fu_sortdir_f sortdir, int parent)
{
  int k,j,i;

  lua_settop(L,0);
  /*   if (!dir) { */
  /*     return 0; */
  /*   } */
  if (sortdir) {
    fu_sort_dir(dir, count, sortdir);
  }

  for (k=0; k<2; ++k) {
    lua_newtable(L);
    for (j=i=0; i<count; ++i) {
      fu_dirent_t * d = dir+i;

      int isdir = d->size == -1;
      /* check for suitable list. */
      if ( !(k ^ (isdir)) ) continue;
      /* check for '.' and '..' */
      if (isdir && !parent && d->name[0] == '.' &&
	  (!d->name[1] || (d->name[1] == '.' && !d->name[2]))) continue;
      lua_pushnumber(L, ++j);
      lua_pushstring(L, d->name);
      lua_rawset(L, k+1);
    }
    lua_pushstring(L, "n");
    lua_pushnumber(L, j);
    lua_settable(L, k+1);
  }
  return lua_gettop(L);
}

/* Return a list of struct {name, size} sorted according to the sortdir
   function. */
static int push_dir_as_struct(lua_State * L, fu_dirent_t * dir, int count,
			      fu_sortdir_f sortdir, int parent)
{
  int i, j, table;

  lua_settop(L,0);
  /*   if (!dir) { */
  /*     return 0; */
  /*   } */
  if (sortdir) {
    fu_sort_dir(dir, count, sortdir);
  }

  lua_newtable(L);
  table = lua_gettop(L);


  for (i=j=0; i<count; ++i) {
    fu_dirent_t * d = dir+i;
    int entry;

    if (!parent && d->size < 0 && d->name[0] == '.' &&
	(!d->name[1] || (d->name[1] == '.' && !d->name[2]))) {
      /* Don't want self to be listed ! */
      continue;
    }

    lua_pushnumber(L, ++j);
    lua_newtable(L);
    entry=lua_gettop(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, dir[i].name);
    lua_settable(L, entry);
    lua_pushstring(L, "size");
    lua_pushnumber(L, dir[i].size);
    lua_settable(L, entry);
      
    lua_rawset(L, table);
  }
  lua_pushstring(L, "n");
  lua_pushnumber(L, j);
  lua_settable(L, table);
  return lua_gettop(L);
}

/* Directory listing
 * usage : dirlist [switches] path
 *
 *   Get sorted listing of a directory. There is to possible output depending
 *   on the -2 switch.
 *   If -2 is given, the function returns 2 lists, one for directory, the other
 *   for files. This list contains file name only.
 *   If -2 switch is ommitted, the function returns one list which contains 
 *   one structure by file. Each structure as two fields: "name" and "size"
 *   which contains respectively the file or directory name, and the size in
 *   bytes of files or -1 for directories.
 *
 * switches:
 *  -2 : returns 2 separate lists.
 *  -S : sort by descending size
 *  -s : sort by ascending size
 *  -n : sort by name
 *  -h : hide parent and root entry.
 */
static int lua_dirlist(lua_State * L)
{
  char rpath[2048];
  const char * path = 0;
  int nparam = lua_gettop(L);
  int count, i;
  fu_dirent_t * dir;
  int two = 0, sort = 0, parent = 1;
  fu_sortdir_f sortdir;

  /* Get parameters */
  for (i=1; i<=nparam; ++i) {
    const char *s = lua_tostring(L, i);
    if (!s) {
      printf("dirlist : Bad parameters #%d\n", i);
      return -1;
    }
    if (s[0] == '-') {
      int j;
      for (j=1; s[j]; ++j) {
	switch(s[j]) {
	case '2':
	  two = s[j];
	  break;
	case 's': case 'S': case 'n':
	  if (!sort || sort==s[j]) {
	    sort = s[j];
	  } else {
	    printf("dirlist : '%c' switch incompatible with '%c'.\n",
		   s[j], sort);
	    return -1;
	  }
	  break;
	case 'h':
	  parent = 0;
	  break;
	default:
	  printf("dirlist : invalid switch '%c'.\n", s[j]);
	  return -1;
	}
      }
    } else if (path) {
      printf("dirlist : Only one path allowed. [%s].\n", s);
      return -1;
    } else {
      path = s;
    }
  }
  if (!path) {
    printf("dirlist : Missing <path> parameter.\n");
    return -1;
  }

  if (!fn_get_path(rpath, path, sizeof(rpath), 0)) {
    printf("dirlist : path to long [%s]\n", path);
    return -2;
  }
  if (!rpath[0]) {
    rpath[0] = '/';
    rpath[1] = 0;
  }

  count = fu_read_dir(rpath, &dir, 0);
  if (count < 0) {
    printf("dirlist : %s [%s] \n", fu_strerr(count), rpath);
    return -3;
  }

  switch(sort) {
  case 's':
    /* 	printf("sort by > size\n"); */
    sortdir = fu_sortdir_by_ascending_size;
    break;
  case 'S':
    /* 	printf("sort by < size\n"); */
    sortdir = fu_sortdir_by_descending_size;
    break;
  case 'n':
    /* 	printf("sort by name\n"); */
    sortdir = fu_sortdir_by_name_dirfirst;
    break;
  default:
    sortdir = 0;
  }

  if (two) {
    /* 	printf("Get 2 lists [%d]\n", count); */
    count = push_dir_as_2_tables(L, dir, count, sortdir, parent);
  } else {
    /* 	printf("Get 1 list [%d]\n", count); */
    count = push_dir_as_struct(L, dir, count, sortdir, parent);
    if (count == 1) {
      lua_pushstring(L,"path");
      lua_pushstring(L,rpath);
      lua_settable(L,1);
    }
  }
  /*   printf("->%d\n", count); */

  if (dir) free(dir);
  return count;
}

#define MAX_DIR 32

extern int filetype_lef;

static int r_path_load(lua_State * L, char *path, unsigned int level,
		       const char * ext, int count)
{
  dirent_t *de;
  int fd = -1;
  char *path_end = 0;
  char dirs[MAX_DIR][32];
  int ndirs;

  //dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%2d,[%s])\n", level, path);

  ndirs = 0;

  if (level == 0) {
    goto error;
  }


  fd = fs_open(path, O_RDONLY | O_DIR);
  if (fd<0) {
    count = -1;
    goto error;
  }

  path_end = path + strlen(path);
  path_end[0] = '/';
  path_end[1] = 0;

  while (de = fs_readdir(fd), de) {
    int type;

    type = filetype_get(de->name, de->size);
    if (type == filetype_dir) {
      strcpy(dirs[ndirs++], de->name);
      if (!ext)
	continue;
    }

    if (ext) {
      int l = strlen(de->name);
      if (ext[0] && stricmp(ext, de->name + l - strlen(ext))) {
	continue;
      }
    } else {
      if (type != filetype_lef) {
	continue;
      }
    }

    strcpy(path_end+1, de->name);
    count++;
    //printf("%d %s\n", count, path);
    lua_pushnumber(L, count);
    lua_pushstring(L, path);
    lua_settable(L, 1);
    //lua_pop(L, 1);

  }

 error:
  if (fd>=0) {
    fs_close(fd);
  }

  // go to subdirs if any
  while (ndirs--) {
    int cnt;
    strcpy(path_end+1, dirs[ndirs]);
    cnt = r_path_load(L, path, level-1, ext, count);
    count = (cnt > 0) ? cnt : 0;
  }

  if (path_end) {
    *path_end = 0;
  }
  /*  dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(%2d,[%s]) = %d\n",
      level, path, count);*/
  return count;
}


static int lua_path_load(lua_State * L)
{
  int max_recurse;
  char rpath[2048];
  char ext[32];
  int use_ext;

  int nparam = lua_gettop(L);

  int i;

  // first parameter is path to search into
  //strcpy(rpath, home);
  if (nparam >= 1)
    strcpy(rpath, lua_tostring(L, 1)/*, sizeof(rpath)*/);
  else {
    strcpy(rpath, home);
    strcat(rpath, "plugins");
  }

  // default parameters
  max_recurse = 10;
  ext[0] = 0;
  use_ext = 0;

  // get possibly next parameters and guess their meaning
  i = 2;
  if (i<=nparam && lua_isstring(L, i)) {
    strcpy(ext, lua_tostring(L, i)/*, sizeof(ext)*/);
    use_ext = 1;
  } 

  i = 3;
  if (i<=nparam && lua_isnumber(L, i)) {
    max_recurse = lua_tonumber(L, i);
    //printf("i = %d\n", i);
  }

  lua_settop(L, 0);
  lua_newtable(L);

  //max_recurse -= !max_recurse;

  //printf("max_recurse = %d\n", max_recurse);

  r_path_load(L, rpath, max_recurse, use_ext? ext : 0, 0);

  return 1;
}


static int lua_driver_load(lua_State * L)
{
  char rpath[2048];

  int nparam = lua_gettop(L);
  int i, err = 0, total = 0;

  for (i=1; i<=nparam; i++) {
    int count;
    //strcpy(rpath, home);
    strcpy(rpath, lua_tostring(L, i));
    count = plugin_load_and_register(rpath);
    if (count < 0) {
      err = 1;
    } else {
      total += count;
    }
  }
  if (!err) {
    lua_pushnumber(L,total);
    return 1;
  }
  return 0;
}

static int lua_thd_pass(lua_State * L)
{
  thd_pass();

  return 0;
}


extern void (* ta_scan_cb) ();
static semaphore_t * framecounter_sema;
void scan_cb()
{
  sem_signal(framecounter_sema);
  ta_scan_cb = NULL;
}

static int lua_framecounter(lua_State * L)
{
  int nparam = lua_gettop(L);
  static int offset;
  int ofc;
  int fc;

  for (;;) {
    ofc = ta_state.frame_counter;
    fc = ofc - offset;

    if (fc)
      break;

    ta_scan_cb = scan_cb;
    sem_wait(framecounter_sema);
    //thd_pass();
  }

  if (nparam && !lua_isnil(L, 1)) {
    /* reset framecounter */
    offset = ofc;
  }

  lua_settop(L, 0);
  lua_pushnumber(L, fc);

  return 1;
}

/* we use different keycodes than kos :
 * raw key codes are simply translated by 256, instead of behing MULTIPLIED !
 * this save a lot of other key code possibilites for later use ... */
static int convert_key(int k)
{
  if (k >= 256)
    return (k>>8) + 256;
  else
    return k;
}

static int lua_peekchar(lua_State * L)
{
  int k;

  /* ingnore any parameters */
  lua_settop(L, 0);

  k = csl_peekchar();

  if (k >= 0) {
    lua_pushnumber(L, convert_key(k));
    return 1;
  }

  return 0;
}

static int lua_getchar(lua_State * L)
{
  int k;

  /* ingnore any parameters */
  lua_settop(L, 0);

  k = csl_getchar();

  lua_pushnumber(L, convert_key(k));

  return 1;
}


static int lua_rawprint(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;

  for (i=1; i<= nparam; i++) {
    csl_putstring(csl_main_console, lua_tostring(L, i));
  }

  return 0;
}

static int lua_consolesize(lua_State * L)
{
  lua_pushnumber(L, csl_main_console->w);
  lua_pushnumber(L, csl_main_console->h);

  return 2;
}


extern int csl_echo; /* console.c */

static int lua_consoleecho(lua_State * L)
{
  int old = csl_echo;
  if (lua_gettop(L) >= 1) {
    csl_echo = lua_tonumber(L,1);
  }
  lua_settop(L,0);
  if (old) {
    lua_pushnumber(L,1);
  }
  return lua_gettop(L);
}

static int lua_toggleconsole(lua_State * L)
{
  lua_pushnumber(L, shell_toggleconsole());

  return 1;
}

static int lua_showconsole(lua_State * L)
{
  lua_pushnumber(L, shell_showconsole());

  return 1;
}

static int lua_hideconsole(lua_State * L)
{
  lua_pushnumber(L, shell_hideconsole());

  return 1;
}

static int lua_console_setcolor(lua_State * L)
{
  int n = lua_gettop(L);
  int i, j, k;
  float colors[4][4];
  int read_access = 0x0F; /* read denied by default */

/*   printf("lua_console_setcolor : n=%d\n",n); */

  /* Get colors */
  j = 1;
  for (k=0; k<4; ++k) {
/*     printf("lua_console_setcolor : do color #%d j=%d\n",k,j); */
    colors[k][0] = 1;
    colors[k][1] = colors[k][2] = colors[k][3] = (k == 2);
    if (j<=n) {
/*       printf("lua_console_setcolor : j<n : process\n"); */
      switch (lua_type(L,j)) {
      case LUA_TNUMBER:
/* 	printf("lua_console_setcolor : found a number\n"); */
	read_access &= ~(1<<k);
	for (i=0; i<4; ++i) {
	  colors[k][i] = lua_tonumber(L,j++);
	}
	break;
	
      case LUA_TTABLE:
/* 	printf("lua_console_setcolor : found a table\n"); */
	read_access &= ~(1<<k);
	for (i=0; i<4; ++i) {
	  lua_rawgeti(L,j,i+1);
	  colors[k][i] = lua_tonumber(L,-1);
	}
	lua_settop(L,n);
	++j;
	break;

      case LUA_TNIL:
      default:
/* 	printf("lua_console_setcolor : found a nil or unknown\n"); */
	++j;
      }
    }
/*     printf("lua_console_setcolor : j=%d [%.2f %.2f %.2f %.2f] %X\n", */
/* 	   j, colors[k][0], colors[k][1] ,colors[k][2], colors[k][3], */
/* 	   read_access); */
  }
  
  /* Set / Get console colors */
  csl_console_setcolor(0,colors[0],colors[1],colors[2],colors[3],read_access);

  /* Write returned values back */
  lua_settop(L,0);
  for (k=0; k<4; ++k) {
    int table;
    lua_newtable(L);
    table = lua_gettop(L);
    for (i=0; i<4; ++i) {
      lua_pushnumber(L, colors[k][i]);
      lua_rawseti(L, table, i+1);
    }
  }
/*   printf("lua_console_setcolor top=%d\n", lua_gettop(L)); */
  return lua_gettop(L);
}

static int copyfile(const char *dst, const char *src,
		    int force, int unlink, int verbose)
{
  int err;
  char *fct;

  SDDEBUG("[%s] : [%s] [%s] %c%c%c\n", __FUNCTION__,  dst, src,
	  'f' ^ (force<<5), 'u' ^ (unlink<<5), 'v' ^ (verbose<<5));

  if (unlink) {
    fct = "move";
    err = fu_move(dst, src, force);
  } else {
    fct = "copy";
    err = fu_copy(dst, src, force);
  }

  if (err < 0) {
    printf("%s : [%s] [%s]\n", fct, dst, fu_strerr(err));
    return -1;
  }

  if (err >= 0 && verbose) {
    printf("%s : [%s] -> [%s] (%d bytes)\n", unlink ? "move" : "copy",
	   src, dst, err);
  }
  
  return -(err < 0);
}

static int get_option(lua_State * L, const char *fct,
		      int * verbose, int * force)
{
  int i, nparam = lua_gettop(L), err = 0;

  for (i=1; i<nparam; ++i) {
    const char * fname = lua_tostring(L, i);
    if (fname[0] == '-') {
      int j;
      for (j=1; fname[j]; ++j) {
	switch(fname[j]) {
	case 'v':
	  if (verbose) {
	    *verbose = 1;
	  } else {
	    ++err;
	  }
	  break;
	case 'f':
	  if (force) {
	    *force = 1;
	  } else {
	    ++err;
	  }
	  break;
	}
      }
    }
  }
  if (err) {
    printf("%s : [invalid option]\n", fct);
  }
  return err ? -1 : 0;
}

static int lua_mkdir(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i, err;
  int verbose = 0;

  err = get_option(L, "mkdir", &verbose, 0);
  if (err) {
    return -1;
  }
  for (i=1; i<= nparam; i++) {
    int e;
    const char * fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;
    
    e = fu_create_dir(fname);
    if (e < 0) {
      printf("mkdir : [%s] [%s]\n", fname, fu_strerr(e));
      ++err;
    } else if (verbose) {
      printf("mkdir : [%s] created\n", fname);
    }
  }

  lua_settop(L,0);
  if (!err) {
    lua_settop(L,0);
    lua_pushnumber(L,1);
    return 1;
  }
  return 0;
}

static int lua_unlink(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i, err;
  int verbose = 0, force = 0;
  
  err = get_option(L, "unlink", &verbose, &force);
  if (err) {
    return 0;
  }
    
  for (i=1; i <= nparam; i++) {
    int e;
    const char *fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;

    e = fu_remove(fname);
    if (e < 0) {
      if (!force) {
	printf("unlink : [%s] [%s].\n", fname, fu_strerr(e));
	++err;
      }
    } else if (verbose) {
      printf("unlink : [%s] removed.\n", fname);
    }
  }
  if (!err) {
    lua_pushnumber(L,1);
    return 1;
  }
  return 0;
}

static int lua_copy(lua_State * L)
{
  char fulldest[1024] , *enddest;
  const int max = sizeof(fulldest);
  int nparam = lua_gettop(L);
  const char *dst = 0, *src = 0;;
  int i, err, cnt, slashed;
  int verbose = 0, force = 0;

  err = get_option(L, "copy", &verbose, &force);
  if (err) {
    return 0;
  }
  
  /* Count file parameters , get [first] and [last] file in [src] and [dst] */
  for (i=1, cnt=0; i <= nparam; i++) {
    const char *fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;
    if (!src) {
      src = fname;
    } else {
      dst = fname;
    }
    ++cnt;
  }
  
  if (cnt < 2) {
    printf("copy : [missing parameter]\n");
    return 0;
  }
  
  /* Remove trialing '/' */
  enddest = fn_get_path(fulldest, dst, max, &slashed);
  if (!enddest) {
    printf("copy : [missing destination].\n");
    return 0;
  }
  
  if (cnt == 2) {
    if (fu_is_dir(fulldest)) {
      if (!fn_add_path(fulldest, enddest, fn_basename(src), max)) {
	printf("copy : [filename too long].\n");
	return 0;
      }
    } else if (slashed) {
      printf("copy : [%s] [not a directory].\n", dst);
      return 0;
    }
    err = copyfile(fulldest, src, force, 0, verbose);
  } else  {
    /* More than 2 files, destination must be a directory */
    if (!fu_is_dir(fulldest)) {
      printf("copy : [%s] [not a directory].\n", dst);
      return 0;
    }
	
    err = 0;
    for (i=1; i<=nparam; ++i) {
      const char *fname = lua_tostring(L, i);
      if (fname == dst) {
	break;
      }
      if (fname[i] == '-') {
	continue;
      }
      if (!fn_add_path(fulldest, enddest, fn_basename(fname), max)) {
	printf("copy : [%s] [filename too long].\n",fname);
	err = -1;
      } else {
	err |= copyfile(fulldest, fname, force, 0, verbose);
      }
    }
  }
  if (!err) {
    lua_settop(L,0);
    lua_pushnumber(L,1);
    return 1;
  }
  return 0;
}

static int lua_dcar(lua_State * L)
{
  int nparam = lua_gettop(L);
  int count=-1, com, value = -1; 
  const char * command=0, *archive=0, *path=0, *error="bad arguments";
  dcar_option_t opt;

  /* Get lua parms */
  if (nparam >= 1) {
    command = lua_tostring(L, 1);
  }
  if (nparam >= 2) {
    archive = lua_tostring(L, 2);
  }
  if (nparam >= 3) {
    path = lua_tostring(L, 3);
  }

  dcar_default_option(&opt);
  opt.in.verbose = 0;
  
  if (!command) {
    command = "?";
  }
  
  for (com = 0; *command; ++command) {
    int c = (*command) & 255;
	
    switch(c) {
    case 'a': case 'c': case 's': case 'x': case 't':
      if (com) {
	error = "Multiple commands";
	goto error;
      }
      com = c;
      break;
    case 'v':
      opt.in.verbose = 1;
      break;
    case 'f':
      break;
    default:
      if (c>='0' && c<='9' && value==-1) {
	value = c - '0';
	while ( (c = command[1] & 255), (c>='0' && c<='9')) {
	  value = value * 10 + (c-'0');
	  ++command;
	}
      } else {
	error = "Invalid command";
	goto error;
      }
    }
  }

  
  switch(com) {
  case 'c': case 'a':
    if (archive && path) {
      int size;
      if (value >= 0) opt.in.compress = value % 10u;
      if (com == 'a' && (size=fu_size(archive), size > 0)) {
	opt.in.skip = size;
      }
      count = dcar_archive(archive, path, &opt);
      if (count < 0) {
	error = opt.errstr;
      }
    }
    break;
	
  case 't':
    error = "not implemented";
    break;
  case 's':
    if (path=archive, path) {
      count = dcar_simulate(path, &opt);
      if (count < 0) {
	error = opt.errstr;
      }
    }
    break;
	
  case 'x':
    if (path && archive) {
      if (value > 0) opt.in.skip = value;
      count = dcar_extract(archive, path, &opt);
      if (count < 0) {
	error = opt.errstr;
      }
    }
    break;
  default:
    error = "bad command : try help dcar";
    break;
  }
  
 error:  
  if (count < 0) {
    printf("dcar : %s\n", error);
    return 0;
  } else if (opt.in.verbose) {
    printf("dcar := %d\n", count);
    if (opt.out.level)
      printf(" level        : %d\n",opt.out.level);
    if (opt.out.entries)
      printf(" entries      : %d\n",opt.out.entries);
    if (opt.out.bytes)
      printf(" data bytes   : %d\n",opt.out.bytes);
    if (opt.out.ubytes && opt.out.cbytes) {
      printf(" compressed   : %d\n", opt.out.cbytes);
      printf(" uncompressed : %d\n", opt.out.ubytes);
      printf(" compression  : %d\n",opt.out.cbytes*100/opt.out.ubytes);
    }
  }
  lua_settop(L,0);
  lua_pushnumber(L,count);
  return 1;
}

static int lua_play(lua_State * L)
{
  int nparam = lua_gettop(L);
  const char * file = 0, *error = 0;
  unsigned int imm = 1;
  int track = 0;
  int isplaying;

  /* Get lua parms */
  if (!nparam) {
    goto ok;
  }

  if (nparam >= 1) {
    file = lua_tostring(L, 1);
  }
  if (nparam >= 2) {
    track = lua_tonumber(L, 2);
  }
  if (nparam >= 3) {
    imm = lua_tonumber(L, 3);
  }
  
  if (!file) {
    error = "missing music file argument";
  } else if (imm > 1) {
    error = "boolean expected";
  } else {
    if (playa_start(file, track-1, imm) < 0) {
      error = "invalid music file";
    }
  }

  if (error) {
    printf("play : %s\n", error);
    return 0;
  }

 ok:
  isplaying = playa_isplaying();
  lua_settop(L,0);
  lua_pushnumber(L,isplaying);
  return 1;
}

static int lua_pause(lua_State * L)
{
  int pause;

  if (lua_gettop(L) < 1 || lua_type(L, 1) == LUA_TNIL) {
    pause = playa_ispaused();
  } else {
    pause = playa_pause(lua_tonumber(L, 1) != 0);
  }
  lua_settop(L,0);
  lua_pushnumber(L,pause);
  return 1;
}

static int lua_fade(lua_State * L)
{
  int nparam = lua_gettop(L);
  int ms = 0;

  if (nparam>=1) {
    ms = 1024.0f * lua_tonumber(L, 1);
  }
  ms = playa_fade(ms);
  lua_settop(L,0);
  lua_pushnumber(L, (float)ms * (1.0f/1024.0f));
  return 1;
}

static int lua_volume(lua_State * L)
{
  int nparam = lua_gettop(L);
  int vol = -1;

  if (nparam >= 1 && lua_type(L,1) != LUA_TNIL) {
    vol = (int)(lua_tonumber(L,1) * 255.0f);
    if (vol > 255) vol = 255;
  }
  vol = playa_volume(vol);
  lua_settop(L,0);
  lua_pushnumber(L,(float)vol / 255.0f);
  return 1;
}

static int lua_stop(lua_State * L)
{
  int nparam = lua_gettop(L);
  const char * error = 0;
  unsigned int imm = 1;

  /* Get lua parms */
  if (nparam >= 1) {
    imm = lua_tonumber(L, 1);
  }

  if (imm > 1) {
    error = "boolean expected";
  }
   
  if (!error) {
    playa_stop(imm);
  } else {
    printf("stop : %s\n", error);
    return 0;
  }
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_playtime(lua_State * L)
{
  unsigned int ms = playa_playtime();

  lua_pushnumber(L, ms / 1024.0f);
  lua_pushstring(L, playa_info_make_timestr(0, ms));
  return 2;
}

static int convert_info(lua_State * L, playa_info_t * info, int update)
{
  char * r = (char *)-1;
  int mask;

  if (!info || !info->valid) {
    return 0;
  }

  mask = info->update_mask | -!!update;
  
  lua_settop(L,0);
  lua_newtable(L);

  lua_pushstring(L,"valid");
  lua_pushnumber(L, info->valid);
  lua_settable(L,1);

  lua_pushstring(L,"update");
  lua_pushnumber(L, info->update_mask);
  lua_settable(L,1);
  
  if (mask & (1<<PLAYA_INFO_BITS)) {
    lua_pushstring(L,"bits");
    lua_pushnumber(L, 1<<(playa_info_bits(info, -1)+3));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_STEREO)) {
    lua_pushstring(L,"stereo");
    lua_pushnumber(L, playa_info_stereo(info, -1));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_FRQ)) {
    lua_pushstring(L,"frq");
    lua_pushnumber(L, playa_info_frq(info, -1));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TIME)) {
    lua_pushstring(L,"time_ms");
    lua_pushnumber(L, (float)playa_info_time(info, -1) / 1024.0f);
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_BPS)) {
    lua_pushstring(L,"bps");
    lua_pushnumber(L, playa_info_bps(info, -1));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_BYTES)) {
    lua_pushstring(L,"bytes");
    lua_pushnumber(L, playa_info_bytes(info, -1));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_DESC)) {
    lua_pushstring(L,"description");
    lua_pushstring(L,playa_info_desc(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_ARTIST)) {
    lua_pushstring(L,"artist");
    lua_pushstring(L,playa_info_artist(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_ALBUM)) {
    lua_pushstring(L,"album");
    lua_pushstring(L,playa_info_album(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TRACK)) {
    lua_pushstring(L,"track");
    lua_pushstring(L,playa_info_track(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TITLE)) {
    lua_pushstring(L,"title");
    lua_pushstring(L,playa_info_title(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_YEAR)) {
    lua_pushstring(L,"year");
    lua_pushstring(L,playa_info_year(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_GENRE)) {
    lua_pushstring(L,"genre");
    lua_pushstring(L,playa_info_genre(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_COMMENTS)) {
    lua_pushstring(L,"comments");
    lua_pushstring(L,playa_info_comments(info, r));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_FORMAT)) {
    lua_pushstring(L,"format");
    lua_pushstring(L,playa_info_format(info));
    lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TIME)) {
    lua_pushstring(L,"time");
    lua_pushstring(L,playa_info_timestr(info));
    lua_settable(L,1);
  }

  return 1;
}

/* update | [file, [track]] */ 
static int lua_music_info(lua_State * L)
{
  int err;
  playa_info_t * info;

  info = playa_info_lock();
  err = convert_info(L,info, lua_tonumber(L,1));
  info->update_mask = 0;
  playa_info_release(info);

  return err;
}

static int lua_music_info_id(lua_State * L)
{
  playa_info_t * info;
  int id = 0;

  info = playa_info_lock();
  if (info) {
    id = info->valid;
    playa_info_release(info);
  }
  lua_settop(L,0);
  lua_pushnumber(L,id);
  return 1;
}


/* THIS IS REALLY DIRTY AND TEMPORARY :)) */
/* ben : This seems OK to me , now ;) */
#include "controler.h"
/* defined in controler.c */

static int lua_cond_connect(lua_State * L)
{
  int connect;
  if (lua_gettop(L) < 1 || lua_type(L,1) == LUA_TNIL) {
    connect = controler_binding(0,0); /* Just read the state. */
  } else {
    /* clear all and change wanted => set wanted */
    connect = controler_binding(-1,lua_tonumber(L, 1));
  }
  lua_pushnumber(L,connect);
  return 1;
}

/* vmu_tools : 2 commands
 * 1) [ b , vmupath, file ] backup vmu to file ]
 * 2) [ r , vmupath, file ] restore file into vmu ]
 */
static int lua_vmutools(lua_State * L)
{
  char success[32];
  char *buffer = 0;
  char * err = 0;
  int len;
  int nparam = lua_gettop(L);
  const char * command, * vmupath, * file;

  if (nparam != 3) {
    err = "Bad number of argument";
    goto error;
  }
  
  command = lua_tostring(L, 1);
  vmupath = lua_tostring(L, 2);
  file    = lua_tostring(L, 3);

  if (!command || !vmupath || !file) {
    err = "Bad argument";
    goto error;
  }

  if ((command[0] != 'b' && command[0] != 'r') || command[1]) {
    err = "Bad <command> argument";
    goto error;
  }

  if (command[0] == 'r') {
    /* Restore */
    printf("vmu_tools : Restoring [%s] from [%s]...\n", vmupath, file);

    buffer = gzip_load(file, &len);
    if (!buffer) {
      err = "File read error";
      goto error;
    }
    if (len != (128<<10)) {
      err = "Bad file size (not 128Kb)";
      goto error;
    }
#ifdef NOTYET
    if (fs_vmu_restore(vmupath, buffer)) {
      err = "Restore failure";
      goto error;
    }
#endif

  } else {
    printf("vmu_tools : Backup [%s] into [%s]...\n",
	   vmupath, file);

    buffer = malloc(128<<10);
    if (!buffer) {
      err = "128K buffer allocation failure";
      goto error;
    }
#ifdef NOTYET
    if (fs_vmu_backup(vmupath, buffer)) {
      err = "Backup failure";
      goto error;
    }
#endif
    if (len = gzip_save(file, buffer, 128<<10), len < 0) {
      err = "File write error";
      goto error;
    }
  }

  sprintf(success, "compression: %d", (len * 100) >> 17);

 error:
  if(buffer) {
    free(buffer);
  }
  printf("vmu_tools : [%s].\n", err ? err : success);

  return err ? -1 : 0;
}

static int lua_vcolor(lua_State * S)
{
  vid_border_color(lua_tonumber(S, 1), lua_tonumber(S, 2), lua_tonumber(S, 3));

  return 0;
}

static int lua_clear_cd_cache(lua_State * L)
{
#ifdef NOTYET
  iso_ioctl(0,0,0);  /* clear CD cache ! */
#endif
  return 0;
}

static int lua_canonical_path(lua_State * L)
{
  const char * path;
  char buffer[1024], *res;

  if (lua_type(L,1) != LUA_TSTRING) {
    printf("canonical_path : bad argument\n");
    return 0;
  }
  path = lua_tostring(L,1);

  /*   printf("canonical [%s]\n", path); */
  res = fn_canonical(buffer, path, sizeof(buffer));
  if (!res) {
    printf("canonical_path : path [%s] too long\n", path);
    return 0;
  }

  /*   printf("--> [%s]\n", res); */
  lua_settop(L,0);
  lua_pushstring(L,res);
  return 1;
}

static int lua_test(lua_State * L)
{
  int result = -1;
  int n = lua_gettop(L);
  const char * test, * fname;

  if (n != 2 ||
      lua_type(L,1) != LUA_TSTRING ||
      lua_type(L,2) != LUA_TSTRING) {
    printf("test : bad arguments\n");
    return 0;
  }

  test = lua_tostring(L,1);
  fname = lua_tostring(L,2);

  if (test[0] == '-' && test[1] && !test[2]) {
    switch(test[1]) {
    case 'e':
      /* Exist */
      result = fu_exist(fname);
      break;
    case 'd':
      /* Directory */
      result = fu_is_dir(fname);
      break;
    case 'f': case 's':
      result = fu_is_regular(fname);
      if (result && test[1] == 's') {
	/* Empty */
	result = fu_size(fname) > 0;
      }
      break;
    }
  }

  if (result < 0) {
    printf("test : invalid test [%s]\n",test);
    result = 0;
  }

  lua_settop(L,0);
  if (result) {
    lua_pushnumber(L,1);
  }
  return lua_gettop(L);
}

extern void vmu_set_text(const char *s); /* vmu68.c */
extern int vmu_set_visual(int visual);
extern int vmu_set_db(int db);

static int lua_vmu_set_text(lua_State * L)
{
  vmu_set_text(lua_tostring(L,1));
  return 0;
}

static int lua_vmu_set_visual(lua_State * L)
{
  int v = (lua_gettop(L)<1 || lua_type(L,1) == LUA_TNIL)
    ? -1
    : lua_tonumber(L,1);
  lua_pushnumber(L, vmu_set_visual(v));
  return 1;
}

static int lua_vmu_set_db(lua_State * L)
{
  int v = (lua_gettop(L)<1 || lua_type(L,1) == LUA_TNIL)
    ? -1
    : lua_tonumber(L,1);
  lua_pushnumber(L, vmu_set_db(v));
  return 1;
}

/* defined in vmu.c (KOS) */
extern volatile int vmu_io_state;
static int lua_vmu_state(lua_State * L)
{
  lua_settop(L,0);
#ifdef NOTYET
  lua_pushnumber(L, vmu_io_state);
#endif
  return 1;
}

/* defined in dreamcat68.c */
extern int dcplaya_set_visual(const char * name);

static int lua_set_visual(lua_State * L)
{
  vis_driver_t * vis = 0;

  if (lua_gettop(L) >= 1) {
    const char *name = lua_tostring(L,1);
    if (dcplaya_set_visual(name)) {
      printf("[set_visual] : [%s] no such visual.\n", name ? name : "none");
    }
  }
  lua_settop(L,0);
  vis = option_visual();
  if (vis) {
    if(vis->common.name) {
      lua_pushstring(L,vis->common.name);
    }
    driver_dereference(&vis->common);
  }
  return lua_gettop(L);
}

static char * print_wrapper_buffer = 0;
static int print_wrapper_buffer_size = 0;
static int print_wrapper_pos = 0;
static float print_wrapper_policy = 0;
static const int print_wrapper_nl = 0;

static void print_wrapper_kill(void)
{
  if (print_wrapper_buffer) {
    free(print_wrapper_buffer);
    print_wrapper_buffer = 0;
  }
  print_wrapper_pos = 0;
  print_wrapper_buffer_size = 0;
}

static void print_wrapper_reset()
{
  if (print_wrapper_buffer) {
    print_wrapper_buffer[0] = 0;
  }
  print_wrapper_pos = 0;
}

static void print_wrapper_alloc(int needed)
{
  if (needed > print_wrapper_buffer_size) {
    char * p;
    int size;

    SDDEBUG("cur-size:[%d] needed:[%d]\n",
	    print_wrapper_buffer_size, needed);
    if (print_wrapper_policy > 0) {
      size = print_wrapper_buffer_size + print_wrapper_policy;
    } else {
      size = (int)((float)print_wrapper_buffer_size
		   * -print_wrapper_policy
		   + 1.0f);
    }
    SDDEBUG("policy -> [%d]\n", size);
    if (size < needed) {
      size = needed;
    }
    SDDEBUG("real -> [%d]\n", size);
    p = realloc(print_wrapper_buffer, size);
    if (p) {
      print_wrapper_buffer = p;
      print_wrapper_buffer_size = size;
    } else {
      SDERROR("failure\n");
    }
  }
}

static int print_wrapper(lua_State * L)
{
  int i, n = lua_gettop(L);

  for (i=1; i<=n; ++i) {
    const char * s;
    int len ,need;

    s = lua_tostring(L,i);
    if (!s || !s[0]) {
      continue;
    }
    
    /* Got a string, calc length +1 for additionnal '\0' */
    len = strlen(s) + 1;
    need = print_wrapper_pos + len + print_wrapper_nl; /* +1 for '/n' */
    print_wrapper_alloc(need);
    if (need > print_wrapper_buffer_size) {
      continue;
    }
    memcpy(print_wrapper_buffer + print_wrapper_pos,
	   s, len - 1);
    print_wrapper_pos += len - 1;
    if (print_wrapper_nl) {
      print_wrapper_buffer[print_wrapper_pos++] = '\n';
    }
    print_wrapper_buffer[print_wrapper_pos] = 0;
  }
  return 0;
}

static int lua_dostring_print(lua_State * L)
{
  const char * s;
  int print_pos;
  int n = lua_gettop(L);
  float policy;
  int   orgsize;

  /* Want a string ! */
  if (n < 1 || lua_type(L,1) != LUA_TSTRING) {
    return 0;
  }
  /* get alloc policy : >0 added < -1.01 multiplier 0:default */
  policy = lua_tonumber(L,2);
  if (policy > -1.01 && policy <= 0) {
    policy = 2048;
  }
  print_wrapper_policy = policy;
  /* get original original buffer size */
  orgsize = lua_tonumber(L,3);

  /* Discard other parameters */
  lua_settop(L,1);

  s = lua_tostring(L,1);
  if (!s || !s[0]) {
    return 0;
  }

  /* Save old print */
  lua_getglobal(L,"print");
  print_pos = lua_gettop(L);

  /* Register new print */
  lua_register(L,"print",print_wrapper);

  /* Do our dobuffer call $$$ Not thread safe... */
  print_wrapper_reset();
  if (orgsize > 0) {
    print_wrapper_alloc(orgsize);
  }
  lua_dobuffer(L, s, strlen(s), "dostring_print");

  /* restore old print */
  lua_pushvalue(L,print_pos);
  lua_setglobal(L,"print");

  /* Finally return the print_wrapper_buffer */
  lua_settop(L,0);

  SDDEBUG("used:%d size:%d\n",
	  print_wrapper_pos,print_wrapper_buffer_size);

  if (print_wrapper_buffer && print_wrapper_buffer[0]) {
    lua_pushstring(L,print_wrapper_buffer);
  }
  print_wrapper_kill();
  return lua_gettop(L);
}

/* Create a driver info table.
 * @return stack pos of table
 */
static int lua_driver_info(lua_State * L, any_driver_t * driver)
{
  int table;
  char type[4];

  lua_newtable(L);
  table = lua_gettop(L);

  /* Type */
  type[0] = driver->type;
  type[1] = driver->type >> 8;
  type[2] = driver->type >> 16;
  type[3] = 0;
  lua_pushstring(L,"type");
  lua_pushstring(L,type);
  lua_settable(L,table);

  /* Version */
  lua_pushstring(L,"version");
  lua_pushnumber(L,driver->version);
  lua_settable(L,table);

  /* Name */
  if (driver->name) {
    lua_pushstring(L,"name");
    lua_pushstring(L,driver->name);
    lua_settable(L,table);
  }

  /* Authors */
  if (driver->authors) {
    lua_pushstring(L,"authors");
    lua_pushstring(L,driver->authors);
    lua_settable(L,table);
  }
  
  /* Description */
  if (driver->description) {
    lua_pushstring(L,"description");
    lua_pushstring(L,driver->description);
    lua_settable(L,table);
  }
  
  /* Commands */
  if (driver->luacommands && driver->luacommands->name) {
    int comsTable;
    struct luashell_command_description * com;
    lua_pushstring(L,"commands");
    lua_newtable(L);
    comsTable = lua_gettop(L);

    for (com=driver->luacommands; com->name; ++com) {
      int comTable;

      lua_pushstring(L,com->name);
      lua_newtable(L);
      comTable = lua_gettop(L);

      if (com->short_name) {
	lua_pushstring(L,"short_name");
	lua_pushstring(L,com->short_name);
	lua_settable(L,comTable);
      }

      if (com->usage) {
	lua_pushstring(L,"usage");
	lua_pushstring(L,com->usage);
	lua_settable(L,comTable);
      }

      if (com->registered) {
	lua_pushstring(L,"registered");
	lua_pushnumber(L,1);
	lua_settable(L,comTable);
      }
      lua_settable(L,comsTable); /* set "name-command" table into "commands" */
    }

    lua_settable(L,table); /* set "commands" table into table */
  }
  return table;
}

/* Create a driver-list info table.
 * @return stack pos of table
 */
static int lua_driver_list_info(lua_State * L, driver_list_t * dl)
{
  int table;
  any_driver_t *d, *next;

  lua_newtable(L);
  table = lua_gettop(L);

  next = dl->drivers;
  while (d = driver_lock(next), d) {
    lua_pushstring(L,d->name);
    lua_driver_info(L, d);
    next = d->nxt;
    driver_unlock(d);
    lua_settable(L,table);
  }
  return table;
}

/* Get driver information.
 * no paramter : return a table indexed by driver list name containing,
 *               driver info table.
 */
static int lua_driver_list(lua_State * L)
{
  if (lua_gettop(L) < 1) {
    /* No parameter : return all drivers. */
    driver_list_reg_t * reg, * dl, *next;
    int table, n;
    lua_settop(L,0);
    lua_newtable(L);
    table = lua_gettop(L);
    reg = driver_lists_lock();
    for (dl = reg, n=0; dl; dl=next, ++n) {
      driver_list_lock(dl->list);
      lua_pushstring(L, dl->name);
      lua_driver_list_info(L, dl->list);
      next = dl->next;
      driver_list_unlock(dl->list);
      lua_settable(L,table);
    }
    driver_lists_unlock(reg);
    return 1;
  } else {
    printf("driver_info : command not implemented !\n");
  }
  return 0;
}


/* Get an array of all driver list indexed by type (inp, vis, etc ...).
 * no paramter : return a table indexed by driver list type name containing
 *               driver lists.
 */
static int lua_get_driver_lists(lua_State * L)
{
  /* return all driver types. */
  driver_list_reg_t * reg, * dl, *next;
  int table, n;
  lua_settop(L,0);
  lua_newtable(L);
  table = lua_gettop(L);
  reg = driver_lists_lock();
  for (dl = reg; dl; dl=next, ++n) {
    driver_list_lock(dl->list);
    lua_pushstring(L, dl->name);
    /*lua_driver_list_info(L, dl->list); */
    lua_pushusertag(L, dl->list, driverlist_tag);
    next = dl->next;
    driver_list_unlock(dl->list);
    lua_settable(L,table);
  }
  driver_lists_unlock(reg);

  return 1;
}

static int lua_filesize(lua_State * L)
{
  const char *fname = lua_tostring(L,1);
  lua_settop(L,0);
  if (fname) {
    int len = fu_size(fname);
    if (len >= 0) {
      lua_pushnumber(L,len);
    }
  }
  return lua_gettop(L);
}

static int lua_filetype(lua_State * L)
{
  const char * major_name, * minor_name;
  int type, file_size;
  const char * fname;

  fname = lua_tostring(L,1);
  if (!fname) {
    return 0;
  }

  /* This is optionnal parameter that allow to skip the fu_is_dir() call.
     It allow to fix a bug with the dcload_fs, which accept to open files
     as directory :(
  */
  if (lua_gettop(L) > 1) {
    file_size = lua_tonumber(L,2);
  } else {
    file_size = -fu_is_dir(fname);
  }

  if (file_size < 0) {
    type = filetype_directory(fname);
  } else {
    type = filetype_regular(fname);
  }
  if (type < 0) {
    return 0;
  }

  if (filetype_names(type, &major_name, &minor_name) < 0) {
    return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L,type);
  lua_pushstring(L,major_name);
  lua_pushstring(L,minor_name);
  return lua_gettop(L);
}

static int lua_filetype_add(lua_State * L)
{
  const char * major_name = 0, * minor_name = 0, * ext_list;
  int type;

  type = lua_type(L,1);
  if (type == LUA_TNUMBER) {
    major_name = filetype_major_name(lua_tonumber(L,1));
  } else if (type == LUA_TSTRING) {
    major_name = lua_tostring(L,1);
  }

  if (!major_name || !major_name[0]) {
    printf("filetype_add : bad or missing major type.\n");
    return 0;
  }

  type = filetype_major_add(major_name);
  if (type < 0) {
    printf("filetype_add : error creating major type [%s]\n", major_name);
    return 0;
  }

  minor_name = lua_tostring(L,2);
  ext_list = lua_tostring(L,3);

  if (minor_name || ext_list) {
    type = filetype_add(type, minor_name, ext_list);
    if (type < 0) {
      printf("filetype_add : error creating minor type\n");
      return 0;
    }
  }

  lua_settop(L,0);
  lua_pushnumber(L,type);
  return 1;
}

static int lua_intop(lua_State * L)
{
  int a,b;
  const char * c;
  if (lua_gettop(L) < 3) {
    return 0;
  }
  a = lua_tonumber(L,1);
  b = lua_tonumber(L,3);
  c = lua_tostring(L,2);
  lua_settop(L,0);

  if (c && c[0] && !c[1]) {
    int err = 0;
    switch(c[0]) {
    case '|':
      a|=b;
      break;
    case '&':
      a&=b;
      break;
    case '^':
      a^=b;
      break;
    case '<':
      a<<=b;
      break;
    case '>':
      a>>=b;
      break;
    case '+':
      a+=b;
      break;
    case '-':
      a-=b;
      break;
    case '*':
      a*=b;
      break;
    case '%':
    case '/':
      if (b) {
	a = c[0]=='%'?a%b:a/b;
	break;
      }
      printf("[intop] : divide by zero.\n");
    default:
      err = -1;
    }
    if (!err) {
      lua_pushnumber(L,a);
    }
  }
  return lua_gettop(L);
}

static const char * image_typestr(int type)
{
  switch(type & 255) {

  case SHAPF_IND1: case SHAPF_IND2: case SHAPF_IND4: case SHAPF_IND8:
    return "indexed";
  case SHAPF_RGB233:
    return "argb0233";
  case SHAPF_GREY8:
    return "greyscale";

  case SHAPF_ARGB2222:
    return "argb2222";

  /* 16 bit per pixel */
  case SHAPF_RGB565:
     return "argb565";

  case SHAPF_ARGB1555:
    return "argb1555";
  case SHAPF_ARGB4444:
    return "argb4444";
  case SHAPF_AIND88:
    return "interlaced-alpha-indexed-88";


  /* 32 bit per pixel */
  case SHAPF_ARGB32:
    return "argb8888";
  default:
    return "?";
  }
}

static int lua_image_info(lua_State * L)
{
  const char * name = lua_tostring(L,1);
  SHAwrapperImage_t * img;
  if (!name || !*name) {
    printf("[image_info] : missing file parameter\n");
    return 0;
  }
  img = SHAwrapperLoadFile(name, 1);
  if (!img) {
    printf("[image_info] : [%s] not an image file.\n", name);
    return 0;
  }

  lua_settop(L,0);
  lua_newtable(L);

  lua_pushstring(L, "format");
  lua_pushstring(L, img->ext + (img->ext[0] == '.'));
  lua_settable(L, 1);

  lua_pushstring(L, "w");
  lua_pushnumber(L, img->width);
  lua_settable(L, 1);

  lua_pushstring(L, "h");
  lua_pushnumber(L, img->height);
  lua_settable(L, 1);

  lua_pushstring(L, "bpp");
  lua_pushnumber(L, SHAPF_BPP(img->type));
  lua_settable(L, 1);

  lua_pushstring(L, "type");
  lua_pushstring(L, image_typestr(img->type));
  lua_settable(L, 1);

  if (img->lutSize > 0) {
    lua_pushstring(L, "lut");
    lua_pushnumber(L, img->lutSize);
    lua_settable(L, 1);
  }
  lua_settop(L,1);

  return lua_gettop(L);
}

static int greaterlog2(int v)
{
  int i;
  for (i=0; i<(sizeof(int)<<3); ++i) {
    if ((1<<i) >= v) {
      return i;
    }
  }
  return -1;
}


static int lua_load_background(lua_State * L)
{
  texid_t texid, srctexid = -1;
  texture_t tmp, * btexture, * stexture = 0;
  SHAwrapperImage_t * img = 0;
  int i;
  int type;
  const char * typestr;
  float dw, dh, orgRatio, u1, v1, w, h;
  int smodulo = 0;

  struct {
    float x,y,u,v;
  } vdef[2];

  texid = texture_get("__background__");
  if (texid < 0) {
    texid = texture_create_flat("__background__",1024,512,0xFFFFFFFF);
  }
  if (texid < 0) {
    printf("load_background : no [__background__] texture.\n");
    return 0;
  }

  switch(lua_type(L,1)) {
  case LUA_TNUMBER:
    srctexid = lua_tonumber(L,1);
    if (srctexid == texid) {
      printf("load_background : using __background__ as source texture.\n");
    } else {
      stexture = texture_lock(srctexid,1);
      if (stexture) {
	smodulo = (1 << stexture->wlog2) - stexture->width;
      }
    }
    break;
  case LUA_TSTRING:
    img = LoadImageFile(lua_tostring(L,1),0);
    if (img) {
      tmp.width = img->width;
      tmp.height = img->height;
      tmp.wlog2 = greaterlog2(img->width);
      tmp.hlog2 = greaterlog2(img->height);
      tmp.addr = img->data;
      stexture = &tmp;
      ARGB32toRGB565(tmp.addr, tmp.addr, tmp.width * tmp.height);
      stexture->format = texture_strtoformat("0565");
      stexture->twiddled = stexture->twiddlable = 0;
    }
    break;
  }

  if (!stexture) {
    if (img) free(img);
    printf("load_background : invalid source image.\n");
    return 0;
  }

  type = 0;
  typestr = lua_tostring(L,2);
  if (typestr) {
    if (!stricmp(typestr,"center")) {
      type = 1;
    } else if (!stricmp(typestr,"tile")) {
      type = 2;
    }
  }

  /* Original aspect ratio */
  w = dw = stexture->width;
  h = dh = stexture->height;
  orgRatio = dh / dw;

  /* $$$ Now we need power of two in all case (twiddle) */
  if (1 || type == 2) {
    /* Tile needs power of 2 dimension */
    dw = (float)(1 << stexture->wlog2);
    dh = (float)(1 << stexture->hlog2);
  }

  /* $$$ Another rule is that texture width must not be less than height */
  if (dw < dh) {
    dw = dh;
  }

  /* $$$ No coordinate must be larger than 1024 */
  if (dw > 1024) dw = 1024;
  if (dh > 1024) dh = 1024;
  
  /* $$$ At last we must not be larger than the buffer */
  if (dh * dw > 1024 * 512) {
    if (dw == dh || dh/dw > orgRatio) {
      /* dw == dh == 1024, restrict dh
	 or New ratio is greater than older, better loose precision on H */
      dh /= 2;
    } else {
      dw /= 2;
    }
  }

  /* Don't need de-twiddle since we are going to commando that texture */
  btexture = texture_fastlock(texid, 1);
  if (!btexture) {
    /* Safety net ... */
    return 0;
  }

  /* $$$ Bis (see above) */
  if (1 || type == 2) {
    btexture->width  = dw;
    btexture->height = dh;
  } else {
    btexture->width  = 1024;
    btexture->height = 512;
  }
  btexture->wlog2  = greaterlog2(btexture->width);
  btexture->hlog2  = greaterlog2(btexture->height);
  btexture->format = stexture->format;
  btexture->twiddled = 0;  /* Force non twiddle */
  texture_twiddlable(btexture);

#if DEBUG
  if (!btexture->twiddlable) {
    SDCRITICAL("[background] : texture [%s] not twiddlable [%dx%d] [%dx%d]\n",
	       btexture->name,
	       btexture->width,btexture->height,
	       1<<btexture->wlog2,  1<<btexture->hlog2);
  }

  if (1) {
    printf("type:[%s]\n", !type ? "scale" : (type==1?"center":"tile"));
    printf("src : [%dx%d] [%dx%d] , modulo:%d, ratio:%0.2f twid:[%d,%d]\n",
	   stexture->width,stexture->height,
	   1<<stexture->wlog2, 1<<stexture->hlog2,
	   smodulo, orgRatio,
	   stexture->twiddlable, stexture->twiddled);

    printf("bkg : [%dx%d] [%dx%d], modulo:%d twid:[%d,%d]\n",
	   btexture->width,btexture->height,
	   1<<btexture->wlog2,  1<<btexture->hlog2,
	   (1<<btexture->wlog2) - btexture->width,
	   btexture->twiddlable, btexture->twiddled);
  }
#endif

  /* $$$ Currently all texture are 16bit. Since blitz don't care about exact
     pixel format blitz is done with ARGB565 format. */
  Blitz(btexture->addr, dw, dh,
	SHAPF_RGB565, ((1<<btexture->wlog2) - dw) * 2,
	stexture->addr, stexture->width, stexture->height,
	SHAPF_RGB565, smodulo * 2);

  if (stexture != &tmp) {
    /* Only twiddled source texture if we really has locked it. */
    /* $$$ ben : Dangerous ?
       The texture is not really locked here ! Anyway this should not make
       crash. In the worse case we'll have a crashed texture while rendering.
       $$$ ben : Dangerous is back:
       There was a probleme, the texture could be destroyed if we unlock it.
       I have removed the above texture_release() and out it here.
    */
    /* VP : don't call twiddle on a texture, it is done automatically at
       rendering time IF necessary */
    /*texture_twiddle(stexture, 1); */
    texture_release(stexture);
  }

  /* Twiddle the texture :) */
  /* VP : don't call twiddle on a texture, it is done automatically at
     rendering time IF necessary */
  /* Ok actually it was a good idea, in this way we don't lock rendering :) */
  texture_twiddle(btexture,1);
  /* Finally unlock the background texture */
  texture_release(btexture);

  if (img) {
    free(img);
  }

  vdef[0].u = vdef[0].v = 0;

  /* Number of pixel per U/V */
  u1 = 1.0f / (float) (1<<btexture->wlog2);
  v1 = 1.0f / (float) (1<<btexture->hlog2);
  
  if (type != 2) {
    vdef[1].u = (float)dw * u1;
    vdef[1].v = (float)dh * v1;

    if (!type) {
      w = 1;
      h = (640.0f / 480.0f) * orgRatio;
      if (h > w) {
	w /= h;
	h = 1;
      }
    } else {
      w /= 640;
      h /= 480;
    }

    vdef[0].x = (1 - w) * 0.5;
    vdef[0].y = (1 - h) * 0.5;
    vdef[1].x = 1 - vdef[0].x;
    vdef[1].y = 1 - vdef[0].y;

  } else {
    u1 = 1.0f / w;
    v1 = 1.0f / h;

    vdef[1].u = 640.0f * u1;
    vdef[1].v = 480.0f * v1;
    vdef[0].x = vdef[0].y = 0;
    vdef[1].x = vdef[1].y = 1;
  }

  lua_settop(L, 0);
  lua_newtable(L);
  for (i=0; i<4; ++i) {
    lua_newtable(L);

    lua_pushnumber(L, vdef[i&1].x);      /* X */
    lua_rawseti(L, 2, 1);

    lua_pushnumber(L, vdef[(i&2)>>1].y); /* Y */
    lua_rawseti(L, 2, 2);

    lua_pushnumber(L, vdef[i&1].u);      /* U */
    lua_rawseti(L, 2, 3);

    lua_pushnumber(L, vdef[(i&2)>>1].v); /* V */
    lua_rawseti(L, 2, 4);

    lua_rawseti(L, 1, i+1);
  }
  return 1;
}


static int lua_frame_to_second(lua_State * L)
{
  lua_pushnumber(L, frame_to_second(lua_tonumber(L,1)));
  return 1;
}


static void controler_button_to_table(lua_State * L, int buttons)
{
  int m,i,table;
  static const char * button_name[16] = {
    "c", "b", "a", "start",
    "up", "down", "left", "right",
    "z", "y", "x", "d",
    "up2", "down2", "left2", "right2"
  };
  
  lua_newtable(L);
  table = lua_gettop(L);
  for (i=0, m=1; i<16; ++i, m <<= 1) {
    if (buttons & m) {
      lua_pushstring(L,button_name[i]);
      lua_pushnumber(L,1);
      lua_rawset(L,table);
    }
  }
  
  if (!(buttons &
	(CONT_DPAD_LEFT | CONT_DPAD_RIGHT | CONT_DPAD_UP | CONT_DPAD_DOWN))) {
    lua_pushstring(L,"center");
    lua_pushnumber(L,1);
    lua_rawset(L,table);
  }
}

static void controler_state_to_table(lua_State * L,
				     const controler_state_t * state)
{
  int table;

  lua_newtable(L);
  table = lua_gettop(L);

  lua_pushstring(L,"buttons");
  controler_button_to_table(L, state->buttons);
  lua_rawset(L,table);

  lua_pushstring(L,"buttons_change");
  controler_button_to_table(L, state->buttons_change);
  lua_rawset(L,table);

  lua_pushstring(L,"elapsed_time");
  lua_pushnumber(L, frame_to_second(state->elapsed_frames));
  lua_rawset(L,table);

  lua_pushstring(L,"rtrig");
  lua_pushnumber(L, (float)state->rtrig * (1/255.0f));
  lua_rawset(L,table);
  lua_pushstring(L,"ltrig");
  lua_pushnumber(L, (float)state->ltrig * (1/255.0f));
  lua_rawset(L,table);
  lua_pushstring(L,"joyx");
  lua_pushnumber(L, (float)state->joyx  * (1/128.0f));
  lua_rawset(L,table);
  lua_pushstring(L,"joyy");
  lua_pushnumber(L, (float)state->joyy  * (1/128.0f));
  lua_rawset(L,table);
  lua_pushstring(L,"joy2x");
  lua_pushnumber(L, (float)state->joy2x  * (1/128.0f));
  lua_rawset(L,table);
  lua_pushstring(L,"joy2y");
  lua_pushnumber(L, (float)state->joy2y  * (1/128.0f));
  lua_rawset(L,table);
}

static int lua_read_controler(lua_State * L)
{
  controler_state_t state;

  if (lua_gettop(L) > 0) {
    int num = lua_tonumber(L,1);
    lua_settop(L,0);
    if (!controler_read(&state, num-1)) {
      controler_state_to_table(L, &state);
    }
  } else {
    int i;
    lua_settop(L,0);
    lua_newtable(L);
    for (i=0; ; ++i) {
      int err = controler_read(&state, i);
      if (err < 0) {
	break;
      } else if (!err) {
	controler_state_to_table(L, &state);
	lua_rawseti(L, 1, i+1);
      }
    }
  }
  return lua_gettop(L);
}

/* VP : moved this from our custom kos 1.5, should we propose
   these functions for kos 1.3 ? */
#include <dc/cdrom.h>

/* $$$ Drive status */
#define CDROM_BUSY    0
#define CDROM_PAUSED  1
#define CDROM_STANDBY 2
#define CDROM_PLAYING 3
#define CDROM_SEEKING 4
#define CDROM_SCANING 5
#define CDROM_OPEN    6
#define CDROM_NODISK  7
#define CDROM_UNKNOWN 0xFF

/* $$$ Disk type */
#define CDROM_DISK_CDDA  0
#define CDROM_DISK_CDROM 0x10
#define CDROM_DISK_CDXA  0x20
#define CDROM_DISK_CDI   0x30
#define CDROM_DISK_GDROM 0x80


static const char * cdrom_statusstr(int status)
{
  const char * str [] = {
    "cdrom busy", "cdrom paused", "cdrom standby", "cdrom playing",
    "cdrom seeking", "cdrom scaning", "cdrom open", "cdrom nodisk",
    "cdrom error"
  };

  status &= 0xFF;
  if (status > 7) {
    status = 8;
  }
  return str[status];
}

static const char * cdrom_drivestr(int status)
{
  status = (status>>8) & 0xFF;
  switch (status) {
  case CDROM_DISK_CDDA:  return "CDDA";
  case CDROM_DISK_CDROM: return "CDROM";
  case CDROM_DISK_CDXA:  return "CDXA";
  case CDROM_DISK_CDI:   return "CDI";
  case CDROM_DISK_GDROM: return "GDROM";
  }
  return "UNKNOWN";
}

static int cd_status;
static int cd_disc_type;
static int cd_disc_id;
static int cd_door_open;

static int cdrom_status()
{
  return cd_status + (cd_disc_type<<8);
}

static int cdrom_check()
{
  int cd_open_change = 0, cd_open = 1;

  cdrom_get_status(&cd_status, &cd_disc_type);

  switch (cd_status & 0xff) {
  case CDROM_PAUSED:
  case CDROM_STANDBY:
    if (!cd_disc_id) {
      cdrom_reinit();
      cd_disc_id++;
    }
  case CDROM_BUSY:
  case CDROM_PLAYING:
  case CDROM_SEEKING:
  case CDROM_SCANING:
  case CDROM_NODISK:
    cd_open = 0;
  case CDROM_OPEN:
    cd_open_change = cd_open ^ cd_door_open;
    cd_door_open = cd_open;
  default:
    /* Invalid status : don't know if door is open or closed. */
    break;
  }

  if (cd_open_change) {
    if (cd_open) {
      cd_disc_id = 0;
    }
  }

  return cd_status + (cd_disc_type<<8);
}

static int lua_cdrom_status(lua_State * L)
{
  static int lastcheck;
  int check = lua_gettop(L) >= 1 && lua_type(L,1) != LUA_TNIL;

  check = check ? cdrom_check() : cdrom_status();
  if (lastcheck != check) {
    lastcheck = check;
  }

  lua_settop(L,0);
  lua_pushstring(L,cdrom_statusstr(check)+6);
  lua_pushstring(L,cdrom_drivestr(check));
  lua_pushnumber(L,cd_disc_id);
/*   printf("check=%d, status='%s', type='%s', id=%d\n", check, */
/* 	 cdrom_statusstr(check)+6, cdrom_drivestr(check),  */
/* 	 cd_disc_id); */
  return 3;
}

static int lua_vmu_file_load(lua_State * L)
{
  int n = lua_gettop(L);
  vmu_trans_hdl_t hdl;
  vmu_trans_status_e status;
  const char * fname, * path;

  if (n != 2) {
    printf("[vmu_file_load] : invalid number of arguments.\n");
    return 0;
  }
  fname = lua_tostring(L,1);
  path = lua_tostring(L,2);
  if (!fname || !fname[0] || !path || !path[0]) {
    printf("[vmu_file_load] : invalid argument.\n");
    return 0;
  }

  hdl = vmu_file_load(fname, path, lua_tonumber(L,3));
  status = vmu_file_status(hdl);

#if DEBUG
  printf("[vmu_file_load] : status : %d\n",status);
#endif
  lua_settop(L,0);
  if (hdl) {
    lua_pushnumber(L,hdl);
  }
  return lua_gettop(L);
}

static int lua_vmu_file_save(lua_State * L)
{
  int n = lua_gettop(L);
  vmu_trans_hdl_t hdl;
  vmu_trans_status_e status;
  const char * fname, * path;

  if (n != 2) {
    printf("[vmu_file_save] : invalid number of arguments.\n");
    return 0;
  }
  fname = lua_tostring(L,1);
  path = lua_tostring(L,2);
  if (!fname || !fname[0] || !path || !path[0]) {
    printf("[vmu_file_save] : invalid argument.\n");
    return 0;
  }

  hdl = vmu_file_save(fname, path, lua_tonumber(L,3));
  status = vmu_file_status(hdl);

#if DEBUG
  printf("[vmu_file_save] : status : %d\n", status);
#endif
  lua_settop(L,0);
  if (hdl) {
    lua_pushnumber(L,hdl);
  }
  return lua_gettop(L);
}

static int lua_vmu_file_stat(lua_State * L)
{
  const char * s;

  s = vmu_file_statusstr(vmu_file_status(lua_tonumber(L,1 )));
  lua_settop(L,0);
  if (s) {
    lua_pushstring(L,s);
  }
  return lua_gettop(L);
}

static int lua_vmu_file_default(lua_State * L)
{
  const char * s;
  if (lua_gettop(L) < 1) {
    lua_getglobal(L,"vmu_no_default_file");
    s = (lua_type(L,1) == LUA_TNIL) ? vmu_file_get_default() : 0;
  } else {
    s = vmu_file_set_default(lua_type(L,1) == LUA_TNIL
			     ? 0
			     : lua_tostring(L,1));
  }
  lua_settop(L,0);
  if (s) lua_pushstring(L,s);
  return lua_gettop(L);
}


static int lua_ramdisk_modified(lua_State * L)
{
  int ret;
  ret = fs_ramdisk_modified();
  if (ret) {
    lua_pushnumber(L,1);
    return 1;
  }
  return 0;
}
  
static int lua_ramdisk_notify_path(lua_State * L)
{
  fs_ramdisk_notify_path(lua_tostring(L,1));
  return 0;
}

/* defined in keyboard.c */
extern volatile int kbd_present;
static int lua_keyboard_present(lua_State * L)
{
  int present = kbd_present;
  int status;

  kbd_present = status = present & 255;
  lua_pushnumber(L,status);
  if (present & 0x100) {
    lua_pushnumber(L,1);
    return 2;
  }
  return 1;
}

static int LUA_setgcthreshold(lua_State * L)
{
  lua_setgcthreshold(L, lua_tonumber(L, 1));
  return 0;
}

static int LUA_getgcthreshold(lua_State * L)
{
  lua_settop(L,0);
  lua_pushnumber(L, lua_getgcthreshold(L));
  return 1;
}

static int LUA_getgccount(lua_State * L)
{
  lua_settop(L,0);
  lua_pushnumber(L, lua_getgccount(L));
  return 1;
}

static int lua_collect(lua_State * L)
{
  luaC_collect(L, (int) lua_tonumber(L, 1));
  //  printf("collect %d\n", (int) lua_tonumber(L, 1));
  return 0;
}

#include "fifo.h"
static int lua_fifo_used(lua_State * L)
{
  lua_pushnumber(L, fifo_used());
  return 1;
}

static int lua_fifo_size(lua_State * L)
{
  lua_pushnumber(L, fifo_size());
  return 1;
}

static int lua_thread_stats(lua_State * L)
{
  kthread_t *np;
  
  lua_settop(L, 0);
  lua_newtable(L);

  LIST_FOREACH(np, &thd_list, t_list) {
    lua_pushnumber(L, np->tid);
    lua_newtable(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, np->label);
    lua_settable(L, 3);

    lua_pushstring(L, "pwd");
    lua_pushstring(L, np->pwd);
    lua_settable(L, 3);

    lua_pushstring(L, "localtime");
    lua_pushnumber(L, ((unsigned int)(np->localtime/1000)));
    lua_settable(L, 3);

    lua_pushstring(L, "prio");
    lua_pushnumber(L, np->prio);
    lua_settable(L, 3);

    lua_pushstring(L, "prio2");
    lua_pushnumber(L, np->prio2);
    lua_settable(L, 3);

    lua_settable(L, 1);
  }

  return 1;
}

static int lua_thread_prio2(lua_State * L)
{
  kthread_t *np;
  int id, prio2;

  id = lua_tonumber(L, 1);
  prio2 = lua_tonumber(L, 2);

  if (prio2 < 1 || prio2 > 64)
    return 0;
  
  LIST_FOREACH(np, &thd_list, t_list) {
    if (np->tid == id) {
      np->prio2 = prio2;
      break;
    }
  }

  return 0;
}

#ifdef MALLOC_DEBUG
extern void CheckUnfrees();
#endif
static int lua_checkmem(lua_State * L)
{
#ifdef MALLOC_DEBUG
  CheckUnfrees();
#endif
  
  return 0;
}

static luashell_command_description_t commands[] = {

  /* system commands */
  { 
    "malloc_stats","ms","system",
    "mallocs_stats : display memory usage (system memory and video memory)\n"
    ,
    SHELL_COMMAND_C, lua_malloc_stats
  },
  {
    "thd_pass",0,0,
    "thd_pass() : sleep for a little while, not consuming CPU time"
    ,
    SHELL_COMMAND_C, lua_thd_pass
  },
  {
    "framecounter", 0, 0,
    "framecounter(clear) : return the screen refresh frame counter, "
    "if clear is set, then clear the framecounter for next time.\n"
    "since there is 60 frame per second, it is important to clear the "
    "framcounter often to avoid numerical imprecision due to use of floating "
    "point number ! (become imprecise after about 24 hours ...)\n"
    "also, this function never returns zero, it will do thd_pass() until "
    "the framecounter is not zero ..."
    ,
    SHELL_COMMAND_C, lua_framecounter
  },
  {
    "vcolor",0,0,
    "vcolor(r, g, b, a)\n"
    ,
    SHELL_COMMAND_C, lua_vcolor
  },
  {
    "clear_cd_cache",0,0,
    "clear_cd_cache()\n"
    ,
    SHELL_COMMAND_C, lua_clear_cd_cache
  },
  {
    "cdrom_status", "cdstat",0,
    "cdrom_status([update]) :"
    " Read CDROM status. If update is set the status is checked otherwise the"
    " function returns last checked status.\n"
    " Returns status,disk,id. Where :\n"
    " id := a number (0:no-disk)\n"
    " status := [busy,paused,standby,playing,seeking,"
    "scaning,open,nodisk,error]\n"
    " disk := [CDDA,CDROM,CDXA,CDI,GDROM,UNKNOWN];\n"
    ,
    SHELL_COMMAND_C, lua_cdrom_status
  },
  { 
    "set_visual",0,0,
    "set_visual([name]) : set/get visual plugin name.\n"
    ,
    SHELL_COMMAND_C, lua_set_visual
  },
  {
    "frame_to_second",0,0,
    "frame_to_second(frames) : "
    " Convert a number of frame (vertical refresh) into seconds.\n"
    ,
    SHELL_COMMAND_C, lua_frame_to_second
  },
  {
    "ramdisk_is_modified",0,0,
    "ramdisk_is_modified() : "
    "Get and clear ramdisk modification status."
    ,
    SHELL_COMMAND_C, lua_ramdisk_modified
  },
  {
    "ramdisk_notify_path",0,0,
    "ramdisk_modify_path(path) : "
    "Set ramdisk modification notify path. nil desactive notification."
    ,
    SHELL_COMMAND_C, lua_ramdisk_notify_path
  },

  /* file commands */
  { 
    "dir_load","dirl","file",
    "dir_load(path, [max_recurse], [extension]) : enumerate all files"
    " (by default all .lef and .lez files) in given path with max_recurse"
    " (default 10) recursion, return an array of file names\n"
    ")",
    SHELL_COMMAND_C, lua_path_load
  },
  { 
    "dirlist",0,0,
    "dirlist [switches] <path>)\n"
    " switches:\n"
    "  -2 : returns 2 separate lists (see below)\n"
    "  -S : sort by descending size\n"
    "  -s : sort by ascending size\n"
    "  -n : sort by name\n"
    "  -h : hide '.' and '..'\n"
    "\n"
    "Get sorted listing of a directory. There is to possible output depending "
    "on the '-2' switch.\n"
    "\n"
    "If -2 is given, the function returns 2 lists, one for directory,"
    "the other for files. This list contains file name only.\n"
    "\n"
    "If -2 switch is ommitted, the function returns one list which contains "
    "one structure by file. Each structure as two fields: \"name\" and "
    "\"size\" which contains respectively the file or directory name, "
    "and the size in bytes of files or -1 for directories.\n"
    ,
    SHELL_COMMAND_C, lua_dirlist
  },
  { 
    "mkdir","md",0,
    "mkdir(dirname) : create new directory\n"
    ,
    SHELL_COMMAND_C, lua_mkdir
  },
  { 
    "unlink","rm",0,
    "unlink(filename) : delete a file (or a directory ?)\n"
    ,
    SHELL_COMMAND_C, lua_unlink
  },
  {
    "dcar","ar",0,
    "dcar(command,archive[,path]) : works on archive\n"
    "  command:\n"
    "    c : create a new archive.\n"
    "    s : simulate a new archive.\n" 
    "    x : extract an existing archive\n"
    "    t : test an existing archive\n"
    "  options:\n"
    "    v : verbose.\n"
    "    f : ignored.\n"
    ,
    SHELL_COMMAND_C, lua_dcar
  },
  {
    "copy","cp",0,    "copy [options] <source-file> <target-file>\n"
    "copy [options] <file1> [<file2> ...] <target-dir>\n"
    "\n"
    "options:\n"
    "\n"
    " -f : force, overwrite existing file\n"
    " -v : verbose\n"
    ,
    SHELL_COMMAND_C, lua_copy
  },
  { 
    "canonical_path","canonical",0,
    "canonical(path) : get canonical file path.\n"
    ,
    SHELL_COMMAND_C, lua_canonical_path
  },
  { 
    "test",0,0,
    "test(switch,file) : various file test.\n"
    "switch is one of :"
    " -e : file exist\n"
    " -d : file exist and is a directory\n"
    " -f : file exist and is a regular file\n"
    " -s : file is not an empty regular file\n"
    ,
    SHELL_COMMAND_C, lua_test
  },
  {
    "filetype",0,0,
    "filetype(filename[, filesize]) : get type, major-name, minor-name of"
    " given file.\n"
    "The filesize parameter is used only to avoid dir/regular file checking."
    " It is neccessary to fix a bug with regular files on /pc that are"
    " allowed to be opened as directory (and the contrary)."
    ,
    SHELL_COMMAND_C, lua_filetype
  },
  {
    "filetype_add",0,0,
    "filetype_add(major [, minor ] ) : add and return a filetype.\n"
    ,
    SHELL_COMMAND_C, lua_filetype_add
  },
  {
    "filesize",0,0,
    "filesize(filename) : return regular file size.\n"
    ,
    SHELL_COMMAND_C, lua_filesize
  },

  /* IO commands */
  { 
    "peekchar", 0, "io",
    "peekchar() : return input key on keyboard if available and return"
    " its value, or nil if none\n"
    ,
    SHELL_COMMAND_C, lua_peekchar
  },
  { 
    "getchar","gc", 0,
    "getchar() : wait for an input key on keyboard and return its value\n"
    ,
    SHELL_COMMAND_C, lua_getchar
  },
  { 
    "cond_connect",0,0,
    "cond_connect(state) : set the connected state of main controller,"
    "return old state.\n"
    ,
    SHELL_COMMAND_C, lua_cond_connect
  },
  {
    "controler_read",0,0,    "controler_read([num]) :"
    " Read either all or a given controler.\n"
    " Returns respectively a table of controler table or a controler table.\n"
    ,
    SHELL_COMMAND_C, lua_read_controler
  },
  {
    "keyboard_present",0,0,
    "keyboard_present() : "
    "Check the presence of keyboard controller.\n"
    "Returns status [0|1], change [nil|1]"
    ,
    SHELL_COMMAND_C, lua_keyboard_present
  },

  /* Console commands */
  { 
    "consolesize","cs","console",
    "consolesize() : return width and height of the console in character\n"
    ,
    SHELL_COMMAND_C, lua_consolesize
  },
  { 
    "toggleconsole","tc",0,
    "toggleconsole() : toggle console visibility, return old state\n"
    ,
    SHELL_COMMAND_C, lua_toggleconsole
  },
  { 
    "showconsole", "sc",0,
    "showconsole() : show console, return old state\n"
    ,
    SHELL_COMMAND_C, lua_showconsole
  },
  { 
    "hideconsole","hc",0,
    "hideconsole() : hide console, return old state\n"
    ,
    SHELL_COMMAND_C, lua_hideconsole
  },
  { 
    "console_setcolor","concolor",0,
    "console_setcolor("
    "[bta, btr, btg, btb]|nil, "
    "[ [bba, bbr, bbg, bbb]|nil, "
    "[ [ta, tr, tg, tb]|nil,"
    "[ [ca, cr, cg, cb]|nil ] ] ]) : set and get console colors.\n"
    " Where :\n"
    "  - btx are background top color components (a,r,g,b)\n"
    "  - bbx are background bottom color components (a,r,g,b)\n"
    "  - tx are text color components (a,r,g,b)\n"
    "  - cx are cursor color components (a,r,g,b)\n"
    " For each color, if the alpha (a) component is nil r,g,b must be ommit"
    " too.\n"
    " The function always returns 12 floats which are  the 4 components of the"
    " 3 colors."
    ,
    SHELL_COMMAND_C, lua_console_setcolor
  },
  { 
    "console_echo","conecho",0,
    "console_echo([echo]) : set and get console echo status.\n"
    "return old status.",
    SHELL_COMMAND_C, lua_consoleecho
  },

  /* Player commands */
  {
    "playa_play","play","playa",
    "play([music-file [,track, [,immediat] ] ]) : "
    "Play a music file or get play status.\n"
    ,
    SHELL_COMMAND_C, lua_play
  },

  {
    "playa_stop","stop",0,
    "stop([immediat]) : stop current music\n"
    ,
    SHELL_COMMAND_C, lua_stop
  },
  {
    "playa_pause","pause",0,
    "pause([pause_status]) : pause or resume current music or read status\n"
    ,
    SHELL_COMMAND_C, lua_pause
  },

  {
    "playa_fade","fade",0,
    "fade([seconds]) : Music fade-in / fade-out.\n"
    " If seconds = 0 or no seconds is missing read current fade status.\n"
    " If seconds > 0 starts a fade-in.\n"
    " If seconds < 0 starts a fade-out.\n"
    ,
    SHELL_COMMAND_C, lua_fade
  },

  {
    "playa_volume","volume",0,
    "volume([volume]) : Get/Set music volume.\n"
    " volume is a value beetween [0..1].\n"
    " if volume is ommited, nil or < 0 no change occurs,\n"
    " else set volume to given value clipped to max (1).\n"
    " Return previous value in both case."
    ,
    SHELL_COMMAND_C, lua_volume
  },

  {
    "playa_playtime","playtime",0,
    "seconds,str = playa_playtime() :\n"
    "Returns current playing time in seconds and "
    "into a hh:mm:ss formated string."
    ,
    SHELL_COMMAND_C, lua_playtime
  },
  {
    "playa_info","info",0,
    "playa_info( [update | [filename [ ,track ] ] ]) :\n"
    "Get music information table."
    ,
    SHELL_COMMAND_C, lua_music_info
  },
  {
    "playa_info_id","info_id",0,
    "playa_info_id() :\n"
    "Get current music identifier."
    ,
    SHELL_COMMAND_C, lua_music_info_id
  },


  /* VMU commands */
  { 
    "vmu_set_text",0,"vmu",
    "vmu_set_text(text) : set text to display on VMS lcd.\n"
    ,
    SHELL_COMMAND_C, lua_vmu_set_text
  },
  { 
    "vmu_set_visual",0,0,
    "vmu_set_visual([fx-number]) : set/get vmu display effects.\n"
    ,
    SHELL_COMMAND_C, lua_vmu_set_visual
  },
  { 
    "vmu_set_db",0,0,
    "vmu_set_db([boolean]) : set/get vmu display decibel scaling.\n"
    ,
    SHELL_COMMAND_C, lua_vmu_set_db
  },
  {
    "vmu_tools", "vmu",0,
    "vmu_tools b vmupath file : Backup VMU to file.\n"
    "vmu_tools r vmupath file : Restore file into VMU.\n"
    ,
    SHELL_COMMAND_C, lua_vmutools
  },
  {
    "vmu_file_load",0,0,
    "vmu_file_load(fname, path [,default]) : Load a vmu archive. Optionnaly"
    " set this file as default vmu file.\n"
    "Returns vmu_transfert_handle."
    ,
    SHELL_COMMAND_C, lua_vmu_file_load
  },
  {
    "vmu_file_save",0,0,
    "vmu_file_save(fname, path [,default]) : Save a vmu archive. Optionnaly"
    " set this file as default vmu file.\n"
    "Returns vmu_transfert_handle."
    ,
    SHELL_COMMAND_C, lua_vmu_file_save
  },
  {
    "vmu_file_stat",0,0,
    "vmu_file_stat(vmu_transfert_handle) : "
    "Get status string of vmu transfert."
    ,
    SHELL_COMMAND_C, lua_vmu_file_stat
  },
  {
    "vmu_file_default",0,0,
    "vmu_file_default([path|nil]) : "
    "Get/Set vmu default file path. Returns path or nil.\n"
    "!!! CAUTION !!! This function respects the vmu_no_default_file"
    " behaviours. If vmu_no_default_file is set vmu_file_default() always"
    " returns nil."
    ,
    SHELL_COMMAND_C, lua_vmu_file_default
  },
  {
    "vmu_state",0,0,
    "vmu_state() : returns vmu IO access state.\n"
    " bit 0 : set if currently reading on vmu.\n"
    " bit 1 : set if currently writing on vmu.\n"
    ,
    SHELL_COMMAND_C, lua_vmu_state
  },

  /* Image command */
  {
    "load_background",0,0,"image"
    "load_background(filename | texure-id [ , type ]) :"
    " load background image.\n"
    " type := [\"scale\" \"center\" \"tile\", default:\"scale\"\n"
    " Returns a table with 4 vertrices { {x,y,u,v} * 4 }.\n"
    ,
    SHELL_COMMAND_C, lua_load_background
  },
  {
    "image_info","imginfo",0,
    "image_info(filename) :\n"
    " Get image information table :\n"
    " {\n"
    "   format  = \"original format\",\n"
    "   type    = \"pixel description\",\n"
    "   w       = width-in-pixel,\n"
    "   h       = height-in-pixel,\n."
    "   bpp     = bit-per-pixel,\n."
    "   lut     = number of entry in color table or nil\n"
    " }\n"
    ,
    SHELL_COMMAND_C, lua_image_info
  },


  /* Garbage collector commands */
  {
    "getgcthreshold",0,"garbage",
    "getgcthreshold :\n"
    "Return current garbage collection threshold value in Kbytes."
    ,
    SHELL_COMMAND_C, LUA_getgcthreshold
  },

  {
    "setgcthreshold", 0,0,
    "setgcthreshold(new_value) :\n"
    "Set current garbage collection threshold value in Kbytes."
    ,
    SHELL_COMMAND_C, LUA_setgcthreshold
  },
  {
    "getgccount",0,0,
    "getgccount :\n"
    "Return approximate allocated blocks by the garbage collector in Kbytes."
    ,
    SHELL_COMMAND_C, LUA_getgccount
  },

  {
    "collect",0,0,
    "collect(step) :\n"
    "Perform the given number of step of garbage collection."
    ,
    SHELL_COMMAND_C, lua_collect
  },


  /* general commands */
  {
    "intop",0,"general",
    "intop(a,op,b) :\n"
    " Perform `a op b' on integer values.\n"
    " op := [|,^,&,>,<,+,-,*,/,%].\n"
    " Where `<' and `>' are arithmetical (signed) left and right shift.\n"
    "  `|',`&' and `^' are bitwise OR, AND and XOR operands.\n"
    "  `%' and `/' are signed modulo and division operands.\n"
    "  e.g: `a = intop(a,'^',-1)' is equivalent to `a = ~a'."
    ,
    SHELL_COMMAND_C, lua_intop
  },
  { 
    "rawprint","rp",0,
    "rawprint( ... ) : "
    "raw print on console (no extra linefeed like with print)\n"
    ,
    SHELL_COMMAND_C, lua_rawprint
  },
  {
    "dostring_print",0,0,
    "dostring_print(string) : "
    "Like dostring but all print are done into a string."
    " Returns printed values into a string."
    ,
    SHELL_COMMAND_C, lua_dostring_print
  },

  /* driver commands */
  { 
    "driver_load", "dl","driver",
    "driver_load(filename) : read a plugin\n"
    ,
    SHELL_COMMAND_C, lua_driver_load
  },
  {
    "driver_info",0,0,
    "driver_info([driver-name]) :\n"
    " Get driver information. If no driver-name is given, the function returns"
    " a table indexed by driver type conatining driver info tables indexed by"
    " driver name.\n"
    " driver info table contains:\n"
    " {\n"
    "   name        = \"driver name\",\n"
    "   type        = \"type of driver\",\n"
    "   version     = number version (major*256+minor),\n"
    "   authors     = authors string coma ',' separated,\n"
    "   description = a short description string,\n"
    "   commands    = lua commands table (indexed by command name)\n"
    " }\n"
    " command is a table containing:\n"
    " {\n"
    "   short_name  = \"short command name\" (optionnal),\n"
    "   usage       = \"usage command\" (optionnal),\n"
    "   type        = \"lua\" | \"C\",\n"
    "   registered  = 1 (set only if command is registered)\n"
    " }\n"
    ,
    SHELL_COMMAND_C, lua_driver_list
  },
  {
    "get_driver_lists",0,0,
    "get_driver_lists() :\n"
    " Return an array of driver list indexed by their type"
    " (inp, vis, etc ...)\n"
    ,
    SHELL_COMMAND_C, lua_get_driver_lists
  },
  {
    "driver_is_same",0,0,
    "driver_is_same(driver1, driver2) :\n"
    " Compare two driver, return non-nil if they are the same.\n"
    ,
    SHELL_COMMAND_C, lua_driver_is_same
  },
  {
    "fifo_used",0,"fifo",
    "fifo_used() :\n"
    " Return used samples in the fifo.\n"
    ,
    SHELL_COMMAND_C, lua_fifo_used
  },
  {
    "fifo_size",0,"fifo",
    "fifo_size() :\n"
    " Return size of the fifo.\n"
    ,
    SHELL_COMMAND_C, lua_fifo_size
  },

  {
    "shell_get_command",0,"shell",
    "shell_get_command() :\n"
    " Get next external command from queue.\n"
    ,
    SHELL_COMMAND_C, lua_get_shell_command
  },

  {
    "checkmem","cm","system",
    "checkmem() :\n"
    " Display allocated memory statistics.\n"
    ,
    SHELL_COMMAND_C, lua_checkmem
  },

  {
    "thread_prio2",0,"system",
    "thread_prio2(tid, prio2) :\n"
    " Set thread prio2 parameter. USE WITH CARE.\n"
    ,
    SHELL_COMMAND_C, lua_thread_prio2
  },

  {
    "thread_stats","ts","system",
    "thread_stats() :\n"
    " Return a list of threads with statistics.\n"
    ,
    SHELL_COMMAND_C, lua_thread_stats
  },

  {0},
};

static int setgcthreshold(lua_State * L)
{
  int v = lua_getgccount(L);
#ifdef DEBUG
  printf("GCCOUNT = %d\n", v);
#endif
/*  lua_setgcthreshold(L, v + 100);*/
  lua_setgcthreshold(L, 4000);
}

static void shell_register_lua_commands()
{
  int i;
  lua_State * L = shell_lua_state;
  const char * last_topic = "nil";

  /* Create our user data type */

  register_driver_type(L);

  // song type
  song_tag = lua_newtag(L);

  /* New garbage collection threshold adaptative behaviour */
/*  lua_pushcfunction(L, setgcthreshold);
  lua_settagmethod(L, LUA_TNIL, "gc");*/

  lua_dostring(L, 
	       "\n function doshellcommand(string)"
	       "\n   dostring(string)"
	       "\n end");

  printf("setting [home] to [%s]\n",home);

  fdynshell_command("home = [[%s]]", home);

  //fdynshell_command("skip_home_userconf=1");

  //lua_dobuffer(L, shell_basic_lua_init, sizeof(shell_basic_lua_init) - 1, "init");

  /* register functions */
  for (i=0; commands[i].name; i++) {
    lua_register(L, 
		 commands[i].name, commands[i].function);
    if (commands[i].short_name) {
      lua_register(L, 
		   commands[i].short_name, commands[i].function);
    }
  }

  /* luanch the init script */
  printf("running init file [%s]\n",initfile);
  lua_dofile(L, initfile);

  /* register helps */
  for (i=0; commands[i].name; i++) {
    char format[128];
    strcpy(format,"addhelp ([[%s]],");
    strcat(format,commands[i].short_name ? "[[%s]]" : "%s");
    strcat(format,",[[%s]],[[ %s ]])"); /* Add white-space on purpose */

    if (commands[i].usage) {
      fdynshell_command(format,
			commands[i].name,
			commands[i].short_name ? commands[i].short_name : "nil",
			last_topic = commands[i].topic
			? commands[i].topic : last_topic,
			commands[i].usage);
    }
  }
}

#if 0
void shell_lua_fputs(const char * s)
{
  printf(s);
}
#endif



static void shutdown()
{
  int i;

  printf("shell: Shutting down dynamic shell\n");

  lua_dostring(shell_lua_state, "dynshell_shutdown()");

  shell_set_command_func(old_command_func);

  lua_close(shell_lua_state);
  
  controler_binding(-1,-1);

  spinlock_lock(&cr_inmutex);
  for (i=0; i<CR_SIZE; i++)
    if (cr[i])
      free(cr[i]);

  sem_destroy(framecounter_sema);
}

shutdown_func_t lef_main()
{
  printf("shell: Initializing dynamic shell\n");

  //luaB_set_fputs(shell_lua_fputs);

  shell_home_path(home, sizeof(home), "");
  shell_home_path(initfile, sizeof(initfile), "lua/init.lua");

  shell_lua_state = lua_open(10*1024);  
  if (shell_lua_state == NULL) {
    printf("shell: error initializing LUA state\n");
    return 0;
  }

  framecounter_sema = sem_create(0);

  lua_baselibopen (shell_lua_state);
  lua_iolibopen (shell_lua_state);
  lua_strlibopen (shell_lua_state);
  lua_mathlibopen (shell_lua_state);
  lua_dblibopen (shell_lua_state);

  shell_register_lua_commands();

  old_command_func = shell_set_command_func(dynshell_command);

  printf("shell: dynamic shell initialized.\n");

  // Return pointer on shutdown function
  return shutdown;
}
