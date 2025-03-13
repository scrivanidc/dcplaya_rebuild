/*
** $Id$
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/

#ifndef lobject_h
#define lobject_h


#include "llimits.h"
#include "lua.h"


#ifdef LUA_DEBUG
#undef NDEBUG
#include <assert.h>
#define LUA_INTERNALERROR(s)	assert(((void)s,0))
#define LUA_ASSERT(c,s)		assert(((void)s,(c)))
#else
#define LUA_INTERNALERROR(s)	/* empty */
#define LUA_ASSERT(c,s)		/* empty */
#endif


#ifdef LUA_DEBUG
/* to avoid warnings, and make sure value is really unused */
#define UNUSED(x)	(x=0, (void)(x))
#else
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif


/* mark for closures active in the stack */
#define LUA_TMARK	6


/* tags for values visible from Lua == first user-created tag */
#define NUM_TAGS	6


/* check whether `t' is a mark */
#define is_T_MARK(t)	((t) == LUA_TMARK)


/* GCValue is a struct type for other garbage collectable types to
   cast into it. They include: TString, Proto, Hash, Closure */
struct GCValue;

#define LUA_VTNumber	0
#define LUA_VTString 	1
#define LUA_VUdata 	2
#define LUA_VClosure 	3
#define LUA_VHash 	4
#define LUA_VProto 	5

#define vttype(o)	(o->value.any->vtype)

typedef struct GCValue {
  short vtype; 			/* the value type, useful for casting */
  short marked;			/* either 0 or 1, or value>1, which means never GC */
  struct GCValue *gcnext;	/* pointers  to the prev/next GCValue, which eventually shall */
  struct GCValue *gcprev;	/* 	cover all garbage collectable values */
} GCValue;

typedef union {
  struct TString *ts;	/* LUA_TSTRING, LUA_TUSERDATA */
  struct Closure *cl;	/* LUA_TFUNCTION */
  struct Hash *a;	/* LUA_TTABLE */
  struct CallInfo *i;	/* LUA_TLMARK */
  Number n;		/* LUA_TNUMBER */
} Value;


/* Macros to access values */
#define ttype(o)        ((o)->ttype)
#define nvalue(o)       ((o)->value.n)
#define tsvalue(o)      ((o)->value.ts)
#define clvalue(o)      ((o)->value.cl)
#define hvalue(o)       ((o)->value.a)
#define infovalue(o)	((o)->value.i)
#define svalue(o)       (tsvalue(o)->str)


typedef struct lua_TObject {
  int ttype;
  Value value;
} TObject;


/*
** String headers for string table
*/

/*
** most `malloc' libraries allocate memory in blocks of 8 bytes. TSPACK
** tries to make sizeof(TString) a multiple of this granularity, to reduce
** waste of space.
*/
#define TSPACK	((int)sizeof(int))

typedef struct TString {
  short vtype;
  short marked;
  struct GCValue *gcnext;
  struct GCValue *gcprev;
  union {
    struct {  /* for strings */
      unsigned long hash;
      int constindex;  /* hint to reuse constants */
    } s;
    struct {  /* for userdata */
      int tag;
      void *value;
    } d;
  } u;
  size_t len;
  struct TString *nexthash;  /* chain for hash table */
  char str[TSPACK];   /* variable length string!! must be the last field! */
} TString;


/*
** Function Prototypes
*/
typedef struct Proto {
  short vtype;
  short marked;
  struct GCValue *gcnext;
  struct GCValue *gcprev;
  Number *knum;  /* Number numbers used by the function */
  int nknum;  /* size of `knum' */
  struct TString **kstr;  /* strings used by the function */
  int nkstr;  /* size of `kstr' */
  struct Proto **kproto;  /* functions defined inside the function */
  int nkproto;  /* size of `kproto' */
  Instruction *code;
  int ncode;  /* size of `code'; when 0 means an incomplete `Proto' */
  short numparams;
  short is_vararg;
  short maxstacksize;
  /* debug information */
  int *lineinfo;  /* map from opcodes to source lines */
  int nlineinfo;  /* size of `lineinfo' */
  int nlocvars;
  struct LocVar *locvars;  /* information about local variables */
  int lineDefined;
  TString  *source;
} Proto;


typedef struct LocVar {
  TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} LocVar;


/*
** Closures
*/
typedef struct Closure {
  short vtype;
  short marked;
  struct GCValue *gcnext;
  struct GCValue *gcprev;
  union {
    lua_CFunction c;  /* C functions */
    struct Proto *l;  /* Lua functions */
  } f;
  short isC;  /* 0 for Lua functions, 1 for C functions */
  short nupvalues;
  TObject upvalue[1];
} Closure;


#define iscfunction(o)	(ttype(o) == LUA_TFUNCTION && clvalue(o)->isC)


typedef struct Node {
  TObject key;
  TObject val;
  struct Node *next;  /* for chaining */
} Node;

typedef struct Hash {
  short vtype;
  short marked;
  struct GCValue *gcnext;
  struct GCValue *gcprev;
  Node *node;
  int htag;
  int size;
  Node *firstfree;  /* this position is free; all positions after it are full */
} Hash;


/* unmarked tables and closures are represented by pointing `mark' to
** themselves
*/
#define isfullymarked(x)	((x)->mark > 0)
#define ishalfmarked(x)		((x)->mark < 0)


/*
** informations about a call (for debugging)
*/
typedef struct CallInfo {
  struct Closure *func;  /* function being called */
  const Instruction **pc;  /* current pc of called function */
  int lastpc;  /* last pc traced */
  int line;  /* current line */
  int refi;  /* current index in `lineinfo' */
} CallInfo;


extern const TObject luaO_nilobject;
extern const char *const luaO_typenames[];


#define luaO_typename(o)	(luaO_typenames[ttype(o)])


lint32 luaO_power2 (lint32 n);
char *luaO_openspace (lua_State *L, size_t n);

int luaO_equalObj (const TObject *t1, const TObject *t2);
int luaO_str2d (const char *s, Number *result);

void luaO_verror (lua_State *L, const char *fmt, ...);
void luaO_chunkid (char *out, const char *source, int len);

#endif
