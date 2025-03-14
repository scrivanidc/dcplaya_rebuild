/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: simple programmatic interface for encoder mode setup
 last mod: $Id$

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#include "codec_internal.h"
#include "registry.h"
#include "modes/modes.h"

#include "os.h"
#include "misc.h"


/* deepcopy all but the codebooks; in this usage, they're static
   (don't copy as they could be big) */
static void codec_setup_partialcopy(codec_setup_info *ci,
				 codec_setup_info *cs){
  int i;

  memcpy(ci,cs,sizeof(codec_setup_info)); /* to get the flat numbers */

  /* codebooks */
  for(i=0;i<ci->books;i++){
    ci->book_param[i]=cs->book_param[i];
  }

  /* time backend settings */
  for(i=0;i<ci->times;i++){
    ci->time_param[i]=_time_P[ci->time_type[i]]->
      copy_info(cs->time_param[i]);
  }

  /* floor backend settings */
  for(i=0;i<ci->floors;i++){
    ci->floor_param[i]=_floor_P[ci->floor_type[i]]->
      copy_info(cs->floor_param[i]);
  }

  /* residue backend settings */
  for(i=0;i<ci->residues;i++){
    ci->residue_param[i]=_residue_P[ci->residue_type[i]]->
      copy_info(cs->residue_param[i]);
  }

  /* map backend settings */
  for(i=0;i<ci->maps;i++){
    ci->map_param[i]=_mapping_P[ci->map_type[i]]->
      copy_info(cs->map_param[i]);
  }
  
  /* mode settings */
  for(i=0;i<ci->modes;i++){
    ci->mode_param[i]=_ogg_calloc(1,sizeof(vorbis_info_mode));
    ci->mode_param[i]->blockflag=cs->mode_param[i]->blockflag;
    ci->mode_param[i]->windowtype=cs->mode_param[i]->windowtype;
    ci->mode_param[i]->transformtype=cs->mode_param[i]->transformtype;
    ci->mode_param[i]->mapping=cs->mode_param[i]->mapping;
  }

  /* psy settings */
  for(i=0;i<ci->psys;i++){
    ci->psy_param[i]=_vi_psy_copy(cs->psy_param[i]);
  }

}

/* encoders will need to use vorbis_info_init beforehand and call
   vorbis_info clear when all done */

int vorbis_encode_init_vbr(vorbis_info *vi,
			   long channels,
			   long rate,
			   
			   float base_quality /* 0. to 1. */
			   ){

  switch(channels){
  case 2:
    return(OV_EIMPL);


    break;
  default:
    return(OV_EIMPL);

    break;
  }
}


int vorbis_encode_init(vorbis_info *vi,
		       long channels,
		       long rate,

		       long max_bitrate,
		       long nominal_bitrate,
		       long min_bitrate){

  long bpch;
  int i,j;
  codec_setup_info *ci=vi->codec_setup;
  codec_setup_info *mode=NULL;
  if(!ci)return(OV_EFAULT);

  vi->version=0;
  vi->channels=channels;
  vi->rate=rate;
  
  vi->bitrate_upper=max_bitrate;
  vi->bitrate_nominal=nominal_bitrate;
  vi->bitrate_lower=min_bitrate;
  vi->bitrate_window=2;

  /* copy a mode into our allocated storage */
  bpch=nominal_bitrate/channels;

  switch(channels){
  case 2:
    //if(rate>40000){

    if(bpch<35000){
      mode=&info_44c_Z;
    }else if(bpch<45000){
      mode=&info_44c_Y;
    }else if(bpch<55000){
      mode=&info_44c_X;
    }else if(bpch<75000){
      mode=&info_44c_A;
    }else if(bpch<90000){
      mode=&info_44c_B;
    }else if(bpch<110000){
      mode=&info_44c_C;
    }else if(bpch<160000){
      mode=&info_44c_D;
    }else{
      mode=&info_44c_E;
    }
    //}

    break;
  default:

    if(bpch<40000){
      mode=&info_44_Z;
    }else if(bpch<50000){
      mode=&info_44_Y;
    }else if(bpch<70000){
      mode=&info_44_X;
    }else if(bpch<90000){
      mode=&info_44_A;
    }else if(bpch<120000){
      mode=&info_44_B;
    }else{
      mode=&info_44_C;
    }
    break;
  }

  /* now we have to deepcopy */
  codec_setup_partialcopy(ci,mode);

  /* adjust for sample rate */
  /* we don't have any floor 0 modes anymore 
  for(i=0;i<ci->floors;i++)
    if(ci->floor_type[i]==0)
    ((vorbis_info_floor0 *)(ci->floor_param[i]))->rate=rate; */

  /* adjust for channels */
  /* but all our mappings use submap zero now! */

  return(0);
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg){
  return(OV_EIMPL);
}
		       
