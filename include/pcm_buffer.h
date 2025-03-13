/**
 * @ingroup dcplaya_pcmbuffer_devel
 * @file    pcm_buffer.h
 * @author  benjamin gerard
 * @date    2002
 * @brief   PCM and Bitstream buffer.
 *
 * $Id$
 */

#ifndef _PCM_BUFFER_H_
#define _PCM_BUFFER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_pcmbuffer_devel PCM and Bitstream buffers
 *  @ingroup  dcplaya_devel
 *  @brief    PCM and Bitstream buffers.
 *
 *  @warning  Not thread safe.
 *  @author   benjamin gerard
 *  @{
 */

/** Current PCM buffer size (in mono PCM).
 */ 
#define PCM_BUFFER_SIZE pcm_buffer_size

/** Current Bitstream buffer size (in bytes).
 */ 
#define BS_SIZE         bs_buffer_size

/** Current PCM buffer.
 */ 
extern short * pcm_buffer;

/** Current PCM buffer size (in mono PCM).
 */ 
extern int pcm_buffer_size;

/** Current Bitstream buffer.
 */ 
extern char  * bs_buffer;

/** Current Bitstream buffer size (in bytes).
 */ 
extern int bs_buffer_size;

/** Initialise PCM and Bitstream buffer.
 *
 *    The pcm_buffer_init() function allocates realloc PCM and Bitstream
 *    buffers. For each buffer the reallocation occurs only if the new size
 *    is greater than the current. If the function failed, old buffer are
 *    preserved.
 *
 * @param  pcm_size  Number of mono PCM to alloc for pcm_buffer.
 *                   0:kill buffer -1:default size -2:Don't change.
 * @param  bs_size   Number of bytes to alloc for bs_buffer.
 *                   0:kill buffer -1:default size -2:Don't change.
 *
 * @return error-code
 * @retval  0 success
 * @retval <0 failure
 *
 * @warning In case of failure you need to read the pcm_buffer_size and
 *          bs_buffer_size since it is posible that the function failed
 *          after reallocting only one of this buffers
 */ 
int pcm_buffer_init(int pcm_size, int bs_size);

/** Shutdown PCM and Bitstream buffer.
 *
 *    The pcm_buffer_shutdown() is an alias for the pcm_buffer_init(0,0) call.
 */
void pcm_buffer_shutdown(void);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif
