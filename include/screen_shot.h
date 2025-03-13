/**
 * @ingroup dcplaya_screen_shot
 * @file    screen_shot.h
 * @author  benjamin gerard
 * @date    2002/09/14
 * @brief   Takes TGA screen shot.
 * 
 * $Id$
 */

#ifndef _SCREEN_SHOT_H_
#define _SCREEN_SHOT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_screen_shot Screen shot
 *  @ingroup  dcplaya_devel
 *  @brief    take a screen shot
 *  @author  benjamin gerard
 *  @{
 */

/** Takes a TGA screen shot.
 *
 *     The screen_shot() function saves current frame buffer into a TGA
 *     file in the DCPLAYA_HOME directory. Filenames will be auto
 *     incremented.
 *
 *  @param  basename  Screen-shot file name without extension.
 *
 *  @return  error-code
 *  @retval  0 success
 *  @retval <0 failure
 */
int screen_shot(const char *basename);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #define _SCREEN_SHOT_H_ */
