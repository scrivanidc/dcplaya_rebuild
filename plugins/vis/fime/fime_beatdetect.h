/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_beatdetect.h
 *  @author  benjamin gerard 
 *  @date    2003/01/19
 *  @brief   FIME beat detection
 *
 *  $Id$
 */ 

#ifndef _FIME_BEATDETECT_H_
#define _FIME_BEATDETECT_H_

int fime_beatdetect_init(void);
void fime_beatdetect_shutdown(void);
float fime_beatdetect_update(void);

#endif /* #define _FIME_BEATDETECT_H_ */

