/**
 * @file    fs_rz.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/07
 * @brief   Gzipped compressed rom disk for KOS file system
 * 
 * $Id$
 */

#ifndef _FS_RZ_H_
# define _FS_RZ_H_

/* Initialize the file system */
int fs_rz_init(const unsigned char * romdisk);

/* De-init the file system */
int fs_rz_shutdown(void);

#endif /* #define _FS_RZ_H_ */
