/*
** $Id$
** Global State
** See Copyright Notice in lua.h
*/

#ifndef lstate_h
#define lstate_h

#include "lobject.h"
#include "lua.h"
#include "luadebug.h"



typedef TObject *StkId;  /* index to stack elements */


/*
** marks for Reference array
*/
#define NONEXT          -1      /* to end the free list */
#define HOLD            -2
#define COLLECTED       -3
#define LOCK            -4


struct Ref {
  TObject o;
  int st;  /* can be LOCK, HOLD, COLLECTED, or next (for free list) */
};


struct lua_longjmp;  /* defined in ldo.c */
struct TM;  /* defined in ltm.h */


typedef struct stringtable {
  int size;
  lint32 nuse;  /* number of elements */
  TString **hash;
} stringtable;



struct lua_State {
  /* thread-specific state */
  StkId top;  /* first free slot in the stack */
  StkId stack;  /* stack base */
  StkId stack_last;  /* last free slot in the stack */
  int stacksize;
  StkId Cbase;  /* base for current C function */
  struct lua_longjmp *errorJmp;  /* current error recover point */
  char *Mbuffer;  /* global buffer */
  size_t Mbuffsize;  /* size of Mbuffer */
  /* global state */
  stringtable strt;  /* hash table for strings */
  stringtable udt;   /* hash table for udata */
  Hash *gt;  /* table for globals */
  struct TM *TMtable;  /* table for tag methods */
  int last_tag;  /* last used tag in TMtable */
  struct Ref *refArray;  /* locked objects */
  int refSize;  /* size of refArray */
  int refFree;  /* list of free positions in refArray */
  GCValue *gcroot;  /* all garbage collectable values form a linked list */
  GCValue *gcptr;  /* current gc traversal pointer */
  short gcstage;  /* current gc stage */
  short gcTM;  /* remember last collected tag */
  int gcalloc, last_gcalloc;  /* it is used to calculate the rate of mem allocation */
  unsigned long GCthreshold;
  unsigned long nblocks;  /* number of `bytes' currently allocated */
  lua_Hook callhook;
  lua_Hook linehook;
  int allowhooks;
};


#endif

