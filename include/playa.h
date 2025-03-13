/**
 * @ingroup  dcplaya_playa_devel
 * @file     playa.h
 * @author   benjamin gerard
 * @brief    music player threads
 *
 * $Id$
 */

#ifndef _PLAYA_H_
#define _PLAYA_H_

#include "extern_def.h"
#include "playa_info.h"

DCPLAYA_EXTERN_C_START


/** @defgroup dcplaya_playa_devel Playa
 *  @ingroup  dcplaya_devel
 *  @brief    music player
 *
 *  @author  benjamin gerard
 *
 *  @{
 */

/** @name Main decoder thread status
 *  @{
 */
#define PLAYA_STATUS_INIT     0
#define PLAYA_STATUS_READY    1
#define PLAYA_STATUS_STARTING 2
#define PLAYA_STATUS_PLAYING  3
#define PLAYA_STATUS_STOPPING 4
#define PLAYA_STATUS_QUIT     5
#define PLAYA_STATUS_ZOMBIE   6
#define PLAYA_STATUS_REINIT   7
/**@}*/

/** @name playa initialization functions
 *  @{
 */ 

/** Initialize the playa.
 *  @ingroup  dcplaya_playa_devel
 */
int playa_init();

/** Shutdown the playa.
 *  @ingroup  dcplaya_playa_devel
 */
int playa_shutdown();

/**@}*/

/** @name playa control functions
 *  @{
 */ 

/** Play a music file. */
int playa_start(const char *fn, int track, int immediat);

/** Stop playing. */
int playa_stop(int flush);

/** Pause or resume play. */
int playa_pause(int v);

/** Get/Set playa volume.
 *  @param  volume  new volume [0..255], -1 for get current volume.
 *  @return previous volume
 */
int playa_volume(int volume);

/** Start a fade in/out or get fade status.
 *  @param  ms  @b 0: query fade status,
 *              @b >0:start a fade-in,
 *              @b <0:start a fade-out.
 *              The absolute value of ms determines fade length in
 *              milli-second (exactly a 1024th of second).
 *  @return previous volume
 */
int playa_fade(int ms);

/**@}*/


/** @name playa query functions
 *  @{
 */ 

/** Get playa plying status.
 *  @return playa plying status.
 *  @retval 1 player is playing
 *  @retval 0 player is not playing
 */
int playa_isplaying();


/** Get playa pause status.
 *  @return playa pause status.
 *  @retval 0 player is not paused
 */
int playa_ispaused(void);

/** Get current music play time.
 *
 * @return the number a millisecond (exactly 1024th of second).
 */
unsigned int playa_playtime();

/** Get playa thread status.
 */
int playa_status();

/** Convert playa thread status to string.
 */
const char * playa_statusstr(int status);

/** Get information on current music or music file.
 *
 * @param  info  Informations are store here.
 * @param  fn    Filename of music to get information from.
 *               0 for current playing music.
 *
 * @todo Incomplete : missing track number. Not implemented properly for
 *       file information !!!
 */
int playa_info(playa_info_t * info, const char *fn);

/** Get playa current buffer.
 * @deprecated To get current playing PCM use the fifo_readbak() function.
 */
void playa_get_buffer(int **b, int *nbSamples, int *counter, int *frq);

/** Get playa current sampling rate.
 */
int playa_get_frq(void);

/**@}*/

/**@}*/


DCPLAYA_EXTERN_C_END

#endif
