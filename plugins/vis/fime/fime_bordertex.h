/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_bordertex.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME border texture
 *
 *  $Id$
 */ 

#ifndef _FIME_BORDERTEX_H_
#define _FIME_BORDERTEX_H_

int fime_bordertex_init(void);
void fime_bordertex_shutdown(void);
int fime_bordertex_add(const char *name, const unsigned short *def16);

#endif /* #define _FIME_BORDERTEX_H_ */
