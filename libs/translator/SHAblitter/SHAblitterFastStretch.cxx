/**
 * @ingroup   SHAblitter
 * @file      SHAblitterFastStretch.cxx
 * @brief     Soft blitter fast stretching implementation.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id$
 */

#include "SHAblitter/SHAblitter.h"
#include "SHAsys/SHAsysTypes.h"
#include <string.h>

#include "sysdebug.h"

#define SHABLITTER_STRETCH_FIX 16

// nDst <= nSrc

template <class T> void SHAblitterFastStretchLine(T * dst, const T * src,
						  int nDst, int srcOverDst)
{
  if (nDst) {
    int i = 0;
    int rem = nDst & 3;
    nDst >>= 2;
    do {
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
    } while (--nDst);
    if (rem&2) {
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      i += srcOverDst;
    }
    if (rem&1) {
      *dst++ = src[i >> SHABLITTER_STRETCH_FIX];
      //      i += srcOverDst;
    }
  }
}

template static
void SHAblitterFastStretchLine (SHAuint8  * dst, const SHAuint8  * src,
				int nDst, int srcOverDst);
template static
void SHAblitterFastStretchLine (SHAuint16 * dst, const SHAuint16 * src,
				int nDst, int srcOverDst);
template static
void SHAblitterFastStretchLine (SHAuint32 * dst, const SHAuint32 * src,
				int nDst, int srcOverDst);
template static
void SHAblitterFastStretchLine (SHAuint64 * dst, const SHAuint64 * src,
				int nDst, int srcOverDst);

static inline void * Inc (const void *v, int nBytes)
{
  return (void *) ((const char *)v + nBytes);
}

template <class T> void SHAblitterFastStretch(T * dst, const T * src,
					      int dstAdd, int srcAdd,
					      int dstW, int dstH,
					      int srcW, int srcH)
{
  if (!(dstH | dstW)) {
    // Some destination pixel : nothing to do
    return; 
  }

  const int srcOverDst = (srcW << SHABLITTER_STRETCH_FIX) / dstW;
  dstAdd += dstW * sizeof(T);
  srcAdd += srcW * sizeof(T);


  if (dstH == srcH) {
    // Fast case : no vertical stretching
    SDDEBUG("Fast case [%d]: no vertical stretching src:[%d %d] dst[%d %d]\n",
	    sizeof(T), srcW,srcAdd,dstW,dstAdd);
    do {
      SHAblitterFastStretchLine (dst, src, dstW, srcOverDst);
      dst = (T *)Inc(dst, dstAdd);
      src = (T *)Inc(src, srcAdd);
    } while (--dstH);
  } else {
    // Vertical streching : see if it is reducing or enlarging.
    SDDEBUG("Fast case [%d]: vertical stretching src:[%d %d] dst[%d %d]\n",
	    sizeof(T), srcW,srcAdd,dstW,dstAdd);

    int srcAdd2;
    int h = dstH;
    
    if (dstH >= srcH) {
      srcAdd2 = 0;
    } else {
      // Reduce. Works for enlarge too.
      int nline = srcH / dstH;
      srcAdd2 = srcAdd * nline;
      srcH -= dstH * nline;
    }
    
    // Bresenham in both emlarge and reduce cases.
    // Bresenham avoid one line multiply by line to calculate source line address
    // Using the same code add very few CPU overhead in reduce case.
    // But it improves instruction cache use.
    int acu = dstH;
    do {
      SHAblitterFastStretchLine (dst, src, dstW, srcOverDst);
      dst = (T *) Inc(dst, dstAdd);
      src = (T *) Inc(src, srcAdd2);
      acu -= srcH;
      if (acu <= 0) {
        src = (T *) Inc(src, srcAdd);
        acu += dstH;
      }
    } while (--h);
  }
}

template static
void SHAblitterFastStretch(SHAuint8  * dst, const SHAuint8  * src,
			   int dstAdd, int srcAdd,
			   int dstW, int dstH,
			   int srcW, int srcH);
template static
void SHAblitterFastStretch(SHAuint16 * dst, const SHAuint16 * src,
			   int dstAdd, int srcAdd,
			   int dstW, int dstH,
			   int srcW, int srcH);
template static
void SHAblitterFastStretch(SHAuint32 * dst, const SHAuint32 * src,
			   int dstAdd, int srcAdd,
			   int dstW, int dstH,
			   int srcW, int srcH);
template static
void SHAblitterFastStretch(SHAuint64 * dst, const SHAuint64 * src,
			   int dstAdd, int srcAdd,
			   int dstW, int dstH,
			   int srcW, int srcH);


void SHAblitter::FastStretch(void)
{
  const int bpp = dst.bytePerPix;

  switch (bpp) {
  case 1:
    SHAblitterFastStretch((SHAuint8 *)dst.data, (SHAuint8 *)src.data,
                          dst.eolSkip, src.eolSkip,
                          dst.width, dst.height,
                          src.width, src.height);
    break;
  case 2:
    SHAblitterFastStretch((SHAuint16 *)dst.data, (SHAuint16 *)src.data,
                          dst.eolSkip, src.eolSkip,
                          dst.width, dst.height,
                          src.width, src.height);
    break;
  case 4:
    SHAblitterFastStretch((SHAuint32 *)dst.data, (SHAuint32 *)src.data,
                          dst.eolSkip, src.eolSkip,
                          dst.width, dst.height,
                          src.width, src.height);
    break;
  case 8:
    SHAblitterFastStretch((SHAuint64 *)dst.data, (SHAuint64 *)src.data,
                          dst.eolSkip, src.eolSkip,
                          dst.width, dst.height,
                          src.width, src.height);
    break;

  default:
    {
    }break;
  }

}

