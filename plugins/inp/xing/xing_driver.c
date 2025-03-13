/** @ingroup dcplaya_inp_driver
 *  @file    xing_driver.c
 *  @author  benjamin gerard 
 *  @author  Dan Potter
 *
 *  $Id$
 */ 

 /* Based on :

   sndmp3.c
   (c)2000 Dan Potter

   An MP3 player using sndstream and XingMP3

   This library is designed to be called from another program in a thread. It
   expects an input filename, and it will do all the setup and playback work.
  
   This requires a working math library for m4-single-only (such as newlib).
   
 */

#define VCOLOR(R,G,B) //vid_border_color(R,G,B)

#include <kos.h>
#include <string.h>

//#include "sndstream.h"
//#include "sndmp3.h"
#include "dcplaya/config.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "id3.h"
#include "sysdebug.h"

/************************************************************************/
#include "mhead.h"		/* From xingmp3 */
#include "port.h"		/* From xingmp3 */

/* Conversion codes: generally you won't play with this stuff */
#define CONV_NORMAL	    0
#define CONV_MIXMONO	    1
#define CONV_DROPRIGHT	    2
#define CONV_DROPLEFT	    3
#define CONV_8BIT	    8 

/* Reduction codes: once again, generally won't play with these */
#define REDUCT_NORMAL	    0
#define REDUCT_HALF	    1
#define REDUCT_QUARTER	    2

/* Bitstream buffer: this is for input data */
static char *bs_ptr;
static int bs_count;

/* PCM buffer: for storing data going out to the SPU */
static short * pcm_ptr;
static int pcm_count;
static int pcm_stereo;

/* MPEG file */
static file_t     mp3_fd;
static int	  frame_bytes;
static MPEG	  mpeg;
static MPEG_HEAD  head;
static int        bitrate;
static DEC_INFO   decinfo;

/* Checks to make sure we have some data available in the bitstream
   buffer; if there's less than a certain "water level", shift the
   data back and bring in some more. */
static int bs_fill(int min_bytes)
{
  /* Make sure we don't underflow */
  if (bs_count < 0) bs_count = 0;

  /* Return if there is enought data in buffer. */
  if (bs_count >= min_bytes) {
    return 0;
  }
  
  /* Shift everything back. */
  memmove(bs_buffer, bs_ptr, bs_count);
  bs_ptr = bs_buffer;
    
  if (mp3_fd>=0) {
    int n;
    n = fs_read(mp3_fd, bs_buffer+bs_count, BS_SIZE - bs_count);
    ///$$$ ben: Because of a KOS bug , this does not work correctly !
    if (n < 0) {
      SDDEBUG("xing : Read error.\n");
      return INP_DECODE_ERROR;
    }

    bs_count += n;
  }
  return (bs_count < min_bytes) ? INP_DECODE_END : 0;
}

static int read_more_bytes(void)
{
  int code;
  /* Pull in some more data (and check for EOF) */
  code = bs_fill(frame_bytes);
  if (code == INP_DECODE_END) {
    SDDEBUG("xing : Read complete.\n");
  }
  return code;
}

static int decode_frame(void)
{
  const unsigned int max_resync = 1<<18; /* 0 = no limit */
  static unsigned int resync = 0;
  IN_OUT	x;
	
  pcm_ptr = pcm_buffer;

  /* Is there enought bytes in mp3-buffer */
  if (bs_count >= frame_bytes) {
    int pcm_align_mask = (1<<(pcm_stereo+1)) - 1;
  
    /* Decode a frame */
    x = audio_decode(&mpeg, bs_ptr, (short *) pcm_buffer);
    if (x.in_bytes <= 0) {
      SDDEBUG("xing : Bad sync in MPEG file [%02X%02x]\n",
	      bs_ptr[0]&255,bs_ptr[1]&255);
      do {
	if (++ resync == max_resync) {
	  SDDEBUG("Resync : maximum resync (%u bytes) threshold reached,\n",
		  resync);
	  return INP_DECODE_ERROR;
	}
	bs_ptr      += 1;
	bs_count    -= 1;
	if (bs_count < frame_bytes) {
	  int code;
	  SDDEBUG("Resyncing : %u bytes\n", resync);
	  code = read_more_bytes();
	  if (code) {
	    SDDEBUG("Resync : failed end of file reach after %u bytes\n",
		    resync);
	  }
	  return code;
	}
	x = audio_decode(&mpeg, bs_ptr, (short *) pcm_buffer);
      } while (x.in_bytes <= 0);
    }

    if (resync) {
      SDDEBUG("Resync after %d skipped bytes\n", resync);
      resync = 0;
    }

    bs_ptr      += x.in_bytes;
    bs_count    -= x.in_bytes;

    /* Check output ... */	  
    if (x.out_bytes & pcm_align_mask) {
      SDERROR("xing: Bad number of output bytes."
	     "%d is not a multiple of %d\n",
	     x.out_bytes, pcm_align_mask+1);
      x.out_bytes &= ~pcm_align_mask;
    }
    pcm_count   = x.out_bytes >> (1+pcm_stereo);
	  
  } else {
    /* Pull in some more data (and check for EOF) */
    return read_more_bytes();
  }
  return 0;
}

/* Open an MPEG stream and prepare for decode */
static int xing_init(const char *fn, playa_info_t * info)
{
  file_t fd;

  SDDEBUG(">> %s('%s')\n", __FUNCTION__, fn);

  /* Open the file */
  mp3_fd = fd = fs_open(fn, O_RDONLY);
  if (fd < 0) {
    SDERROR("xing : open input file [%s] failed.\n", fn);
    return -1;
  }
  playa_info_bytes(info,fs_total(fd));
	
  /* Allocate buffers */
  bs_ptr = bs_buffer; bs_count = 0;
  pcm_ptr = pcm_buffer; pcm_count = 0;
	
  /* Fill bitstream buffer */
  if (bs_fill(8)) {
    SDERROR("xing : can't read file header.\n");
    goto errorout;
  }
  frame_bytes = 0;

  /* Are we looking at a RIFF file? (stupid Windows encoders) */
#if 0 /* Do not need thsi code anymore : rsync will do it */
  
  if (!memcmp(bs_ptr,"RIFF",4)) {
    unsigned int riffSize, riffCount;
    int found = 0;
    riffSize = (bs_ptr[4] & 255);
    riffSize += (bs_ptr[5] & 255) << 8;
    riffSize += (bs_ptr[6] & 255) << 16;
    riffSize += (bs_ptr[7] & 255) << 24;
    SDDEBUG("xing: Skipping stupid RIFF header [%u bytes]\n", riffSize);

    bs_ptr += 8;
    bs_count -= 8;

    /* Found a RIFF header, scan through it until we find the data section */
    for (riffCount=riffSize; riffCount>=4; --riffCount) {
      /* Need at least 8 bytes for 'data' chunk. */
      if (bs_fill(8)) {
	break;
      }

      found = !memcmp(bs_ptr,"data",4);
      if (found) {
	unsigned int dataSize;
	dataSize = (bs_ptr[4] & 255);
	dataSize += (bs_ptr[5] & 255) << 8;
	dataSize += (bs_ptr[6] & 255) << 16;
	dataSize += (bs_ptr[7] & 255) << 24;
	SDDEBUG("xing : RIFF 'data' [%u bytes] found.\n",dataSize);
	bs_ptr += 8;
	bs_count -= 8;
	playa_info_bytes(info, dataSize);
	break;
      }
      ++riffCount;
      ++bs_ptr;
      --bs_count;
    }
    if (!found) {
      SDERROR("xing : 'data' chunk not found within 'RIFF' header.\n");
      goto errorout;
    }
  }

  if (((uint8)bs_ptr[0] != 0xff) && (!((uint8)bs_ptr[1] & 0xe0))) {
    SDERROR("xing : Definitely not an MPEG file [%02x%02x]\n",
	    (uint8)bs_ptr[0], (uint8)bs_ptr[1]);
    goto errorout;
  }

#endif

  /* Initialize MPEG engines */
  mpeg_init(&mpeg);
  mpeg_eq_init(&mpeg);

  /* Parse MPEG header */
  {
    int forward;
    bs_fill(BS_SIZE); /* Fill as much data as possible, no error check ! */
    SDDEBUG("xing: Find info within %d bytes.\n", bs_count);
    
    frame_bytes = head_info3(bs_ptr, bs_count, &head, &bitrate, &forward);
    SDDEBUG("xing: forward = %d.\n", forward);
    bs_ptr += forward;
    bs_count -= forward;
  }
  if (frame_bytes == 0) {
    SDERROR("xing: Bad or unsupported MPEG file.\n");
    goto errorout;
  }

  /* Initialize audio decoder */
  /* $$$ Last parameters looks like cut frequency :
     Dim said to me 40000 is a good value */ 
  if (!audio_decode_init(&mpeg, &head, frame_bytes,
			 REDUCT_NORMAL, 0, CONV_NORMAL, 40000)) {
    SDERROR("xing : failed to initialize decoder\n");
    goto errorout;
  }
  audio_decode_info(&mpeg, &decinfo);

  if (decinfo.channels < 1 || decinfo.channels > 2 || decinfo.bits != 16) {
    SDERROR("xing: unsupported audio output format\n");
    goto errorout;
  }

  {
    static char *layers[] = { "invalid", "layer 3", "layer 2", "layer 1" };
    static char *modes[] = { " stereo", " joint-stereo", " dual", " mono" };
    static char desc[32];
    strcpy(desc, layers[head.option]);
    strcat(desc, modes[head.mode]);
    playa_info_desc(info, desc);
  }
	
  /* Copy decoder PCM info */ 
  playa_info_bps(info, bitrate); //$$$kbps or bps ???
  playa_info_frq(info, decinfo.samprate);
  playa_info_bits(info, decinfo.bits);
  playa_info_stereo(info, decinfo.channels-1);

  if (bitrate > 0) {
    unsigned long long ms;
    ms = playa_info_bytes(info, -1);
    ms <<= 13;
    ms /= bitrate;
    playa_info_time(info, ms);
  }

  pcm_ptr    = pcm_buffer;
  pcm_count  = 0;
  pcm_stereo = decinfo.channels-1;

  SDDEBUG("<< %s('%s') := [0]\n",  __FUNCTION__, fn);
  return 0;

 errorout:
  SDERROR("<< %s('%s') := [-1]\n", __FUNCTION__, fn);
  if (fd>=0) fs_close(fd);
  frame_bytes = 0;
  mp3_fd = -1;
  return -1;
}

static void xing_shutdown()
{
  SDDEBUG("%s\n", __FUNCTION__);
  if (mp3_fd >= 0) {
    fs_close(mp3_fd);
    mp3_fd = -1;
  }
}


/************************************************************************/

static int sndmp3_init(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);
  mp3_fd = -1;
  return 0;
}

/* Start playback (implies song load) */
int sndmp3_start(const char *fn, int track, playa_info_t *info)
{
  int err;
  SDDEBUG("%s('%s', %d)\n", __FUNCTION__, fn, track);
  track=track;
  /* Initialize MP3 engine */
  if(id3_info(info, fn)<0) {
    playa_info_free(info);
  }
  err = pcm_buffer_init(-1,-1);
  err = err || xing_init(fn, info);
  if (err) {
    pcm_buffer_shutdown();
    err = -1;
  }
  return err;
}

/* Stop playback (implies song unload) */
static int sndmp3_stop(void)
{
  SDDEBUG("%s()\n", __FUNCTION__);
  xing_shutdown();
  pcm_buffer_shutdown();
  return 0;
}

/* Shutdown the player */
static int sndmp3_shutdown(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);
  sndmp3_stop();
  return 0;
}

static int sndmp3_decoder(playa_info_t * info)
{
  int n = 0;
	
  if (mp3_fd<0) {
    return INP_DECODE_ERROR;
  }
  
  /* No more pcm : decode next mp3 frame
   * No more frame : it is the end
   */
  if (!pcm_count) {
    int status = decode_frame();
    /* End or Error */
    if (status & INP_DECODE_END) {
      return status;
    }
  }

  if (pcm_count > 0) {
    /* If we have some pcm , send them to fifo */
    if (pcm_stereo) {
      n = fifo_write((int *)pcm_ptr, pcm_count);
    } else {
      n = fifo_write_mono(pcm_ptr, pcm_count);
    }
    
    if (n > 0) {
      pcm_ptr   += (n<<pcm_stereo);
      pcm_count -= n;
    } else if (n < 0) {
      return INP_DECODE_ERROR;
    }
  }
  return -(n > 0) & INP_DECODE_CONT;
}

static int sndmp3_info(playa_info_t *info, const char *fname)
{
  return id3_info(info, fname);
}

static driver_option_t * sndmp3_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

static inp_driver_t xing_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "xing",               /* Driver name */
    "Benjamin Gerard, "   /* Driver authors */
    "Dan Potter",
    "Xing Technology "
    "MPEG I layer "
    "I,II,III decoder",  /**< Description */
    0,                   /**< DLL handler */
    sndmp3_init,         /**< Driver init */
    sndmp3_shutdown,     /**< Driver shutdown */
    sndmp3_options,      /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /* User Id */
  ".mp3\0.mp2\0",         /* EXtension list */

  sndmp3_start,
  sndmp3_stop,
  sndmp3_decoder,
  sndmp3_info,
};

EXPORT_DRIVER(xing_driver)

