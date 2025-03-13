/**
 * @ingroup  dcplaya_draw
 * @file     ta.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @date     2002/11/22
 * @brief    draw tile accelarator interface
 *
 * $Id$
 */

/* $$$ ben hacks :
 * Set the draw_current_flags read_only except in the ta.c.
 */
#define _IN_TA_C_

#include "draw/ta.h"
#include "draw/vertex.h"
#include "draw/texture.h"

/** TA polygon. */
typedef struct {
  uint32 word1;
  uint32 word2;
  uint32 word3;
  uint32 word4;
} ta_poly_t;

/* Harcoded values for TA polygon command. */
static const ta_poly_t poly_table[4] = {
  /* TO   texture     opacity                        */
  /* ----------------------------------------------- */
  /* 00   no         transparent                     */
  { 0x82840012, 0x90800000, 0x949004c0, 0x00000000 },
  /* 01   no         opaque                          */
  { 0x80840012, 0x90800000, 0x20800440, 0x00000000 },
  /* 10   yes        transparent                     */
  { 0x8284000a, 0x92800000, 0x949004c0, 0x00000000 },
  /* 11   yes        opaque                          */
  { 0x8084000a, 0x90800000, 0x20800440, 0x00000000 },
};

/** Current value for TA polygon commands */
static ta_hw_poly_t cur_poly;
int draw_current_flags = DRAW_INVALID_FLAGS;

static const int draw_zwrite  = TA_ZWRITE_ENABLE;
static const int draw_ztest   = TA_DEPTH_GREATER;
static const int draw_culling = TA_CULLING_CCW;

static void make_poly_hdr(ta_hw_poly_t * poly, int flags)
{
  const ta_poly_t * p;
  texture_t * t;
  texid_t texid;
  int idx;
  uint32 word3, word4;

  t = 0;
  texid = DRAW_TEXTURE(flags);
  if (texid != DRAW_NO_TEXTURE) {
    t = texture_fastlock(texid, 0);
  }

  idx = 0
	| ((t != 0) << 1)
	| ((flags >> DRAW_OPACITY_BIT) & 0x1);

  p = poly_table + idx;
  poly->word1 = p->word1;
  poly->word2 = p->word2;
  word3 = p->word3;
  word4 = 0;
  if (t) {
    int tformat = t->format;

    /* re-twiddle the texture if necessary */
    if (t->twiddled != t->twiddlable)
      texture_twiddle(t, t->twiddlable);

    if (t->twiddled)
      tformat &= ~1;
    
    /* $$$ does not match documentation !!!  */
    word3 |= ((DRAW_FILTER(flags)^DRAW_FILTER_MASK)) << (13-DRAW_FILTER_BIT);
    word3 |= (t->wlog2-3) << 3;
    word3 |= (t->hlog2-3);
    /* $$$ does not match documentation !! */
    word4 = (tformat<<26) | (t->ta_tex >> 3);

    texture_release(t);


  }
  poly->word3 = word3;
  poly->word4 = word4;
}

/* void draw_poly_hdr(ta_hw_poly_t * poly, int flags) */
/* { */
/*   make_poly_hdr(poly, draw_current_flags = flags); */
/* } */

void draw_set_flags(int flags)
{
  if (flags != draw_current_flags) {
	make_poly_hdr(&cur_poly, draw_current_flags = flags);
	ta_lock();
	ta_commit32_inline(&cur_poly);
	ta_unlock();
  }
}

enum { CLOSE, OPAQUE, TRANSLUCENT };

static int draw_current_render_mode;

#include "kos/sem.h"
static semaphore_t * draw_sema;

unsigned int draw_frame_counter;

void draw_lock()
{
  sem_wait(draw_sema);
}

int draw_trylock()
{
  return sem_trywait(draw_sema);
}

void draw_unlock()
{
  sem_signal(draw_sema);
}

int draw_init_render(void)
{
  ta_init(TA_LIST_OPAQUE_POLYS|TA_LIST_TRANS_POLYS, TA_POLYBUF_32, 1024*1024);
  draw_frame_counter = ta_state.frame_counter;
  draw_current_render_mode = CLOSE;


  /* VP : added pvr dma init for now, that's all we use from kos' PVR code */
  pvr_dma_init();

  draw_sema = sem_create(1);

  return 0;
}

unsigned int draw_open_render(void)
{
  unsigned int oldframe = draw_frame_counter;

  switch(draw_current_render_mode) {
  case OPAQUE:
  case TRANSLUCENT:
    draw_close_render();
  case CLOSE:
    ta_begin_render();
    draw_lock();
    pvr_dummy_poly(TA_OPAQUE);
    draw_frame_counter = ta_state.frame_counter;
    /* Commit dummy polygon. */
    draw_current_flags = DRAW_INVALID_FLAGS;
    draw_set_flags(DRAW_OPAQUE);
    draw_current_render_mode = OPAQUE;
  }

  return draw_frame_counter - oldframe;
}

void draw_translucent_render(void)
{
  switch(draw_current_render_mode) {
  case CLOSE:
    draw_open_render();
  case OPAQUE:
    ta_commit_eol();
    pvr_dummy_poly(TA_TRANSLUCENT);
    draw_current_flags = DRAW_INVALID_FLAGS;
    draw_set_flags(DRAW_TRANSLUCENT);
    draw_current_render_mode = TRANSLUCENT;
  case TRANSLUCENT:
    break;
  }
}

void draw_close_render(void)
{
  switch(draw_current_render_mode) {
  case OPAQUE:
    draw_translucent_render();
  case TRANSLUCENT:
    ta_commit_eol();
    ta_finish_frame();
    draw_unlock();
    draw_current_render_mode = CLOSE;
  case CLOSE:
    break;
  }
}
