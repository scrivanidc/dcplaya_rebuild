
#include "ffmpeg.h"

static void codec_register_all(void)
{
    static int inited = 0;
    
    if (inited != 0)
	return;
    inited = 1;


#if 1

    /* encoders */
#ifdef CONFIG_ENCODERS
    register_avcodec(&ac3_encoder);
    register_avcodec(&mp2_encoder);
#ifdef CONFIG_MP3LAME
    register_avcodec(&mp3lame_encoder);
#endif
#ifdef CONFIG_VORBIS
    register_avcodec(&oggvorbis_encoder);
    register_avcodec(&oggvorbis_decoder);
#endif
#ifdef CONFIG_FAAC
    register_avcodec(&faac_encoder);
#endif
    register_avcodec(&mpeg1video_encoder);
//    register_avcodec(&h264_encoder);
#ifdef CONFIG_RISKY
    register_avcodec(&mpeg2video_encoder);
    register_avcodec(&h263_encoder);
    register_avcodec(&h263p_encoder);
    register_avcodec(&flv_encoder);
    register_avcodec(&rv10_encoder);
    register_avcodec(&mpeg4_encoder);
    register_avcodec(&msmpeg4v1_encoder);
    register_avcodec(&msmpeg4v2_encoder);
    register_avcodec(&msmpeg4v3_encoder);
    register_avcodec(&wmv1_encoder);
    register_avcodec(&wmv2_encoder);
    register_avcodec(&svq1_encoder);
#endif
    register_avcodec(&mjpeg_encoder);
    register_avcodec(&ljpeg_encoder);
    register_avcodec(&huffyuv_encoder);
    register_avcodec(&asv1_encoder);
    register_avcodec(&asv2_encoder);
    register_avcodec(&ffv1_encoder);
    register_avcodec(&zlib_encoder);
    register_avcodec(&dvvideo_encoder);
#endif /* CONFIG_ENCODERS */
    register_avcodec(&rawvideo_encoder);
    register_avcodec(&rawvideo_decoder);

    /* decoders */
#ifdef CONFIG_DECODERS
#ifdef CONFIG_RISKY
    register_avcodec(&h263_decoder);
    register_avcodec(&h261_decoder);
    register_avcodec(&mpeg4_decoder);
    register_avcodec(&msmpeg4v1_decoder);
    register_avcodec(&msmpeg4v2_decoder);
    register_avcodec(&msmpeg4v3_decoder);
    register_avcodec(&wmv1_decoder);
    register_avcodec(&wmv2_decoder);
    register_avcodec(&h263i_decoder);
    register_avcodec(&flv_decoder);
    register_avcodec(&rv10_decoder);
    register_avcodec(&rv20_decoder);
    register_avcodec(&svq1_decoder);
    register_avcodec(&svq3_decoder);
    register_avcodec(&wmav1_decoder);
    register_avcodec(&wmav2_decoder);
    register_avcodec(&indeo3_decoder);
#ifdef CONFIG_FAAD
    register_avcodec(&aac_decoder);
    register_avcodec(&mpeg4aac_decoder);
#endif
#endif
    register_avcodec(&mpeg1video_decoder);
    register_avcodec(&mpeg2video_decoder);
    register_avcodec(&mpegvideo_decoder);
#ifdef HAVE_XVMC
    register_avcodec(&mpeg_xvmc_decoder);
#endif
    register_avcodec(&dvvideo_decoder);
    register_avcodec(&mjpeg_decoder);
    register_avcodec(&mjpegb_decoder);
    register_avcodec(&sp5x_decoder);
/*     register_avcodec(&mp2_decoder); */
/*     register_avcodec(&mp3_decoder); */
    register_avcodec(&mace3_decoder);
    register_avcodec(&mace6_decoder);
    register_avcodec(&huffyuv_decoder);
    register_avcodec(&ffv1_decoder);
    register_avcodec(&cyuv_decoder);
    register_avcodec(&h264_decoder);
    register_avcodec(&vp3_decoder);
    register_avcodec(&theora_decoder);
    register_avcodec(&asv1_decoder);
    register_avcodec(&asv2_decoder);
    register_avcodec(&vcr1_decoder);
    register_avcodec(&cljr_decoder);
    register_avcodec(&fourxm_decoder);
    register_avcodec(&mdec_decoder);
    register_avcodec(&roq_decoder);
    register_avcodec(&interplay_video_decoder);
    register_avcodec(&xan_wc3_decoder);
    register_avcodec(&rpza_decoder);
    register_avcodec(&cinepak_decoder);
    register_avcodec(&msrle_decoder);
    register_avcodec(&msvideo1_decoder);
    register_avcodec(&vqa_decoder);
    register_avcodec(&idcin_decoder);
    register_avcodec(&eightbps_decoder);
    register_avcodec(&smc_decoder);
    register_avcodec(&flic_decoder);
    register_avcodec(&truemotion1_decoder);
    register_avcodec(&vmdvideo_decoder);
    register_avcodec(&vmdaudio_decoder);
    register_avcodec(&mszh_decoder);
    register_avcodec(&zlib_decoder);
#ifdef CONFIG_AC3
    register_avcodec(&ac3_decoder);
#endif
    register_avcodec(&ra_144_decoder);
    register_avcodec(&ra_288_decoder);
    register_avcodec(&roq_dpcm_decoder);
    register_avcodec(&interplay_dpcm_decoder);
    register_avcodec(&xan_dpcm_decoder);
    register_avcodec(&qtrle_decoder);
    register_avcodec(&flac_decoder);
#endif /* CONFIG_DECODERS */

#ifdef AMR_NB
    register_avcodec(&amr_nb_decoder);
#ifdef CONFIG_ENCODERS
    register_avcodec(&amr_nb_encoder);
#endif //CONFIG_ENCODERS
#endif /* AMR_NB */

#ifdef AMR_WB
    register_avcodec(&amr_wb_decoder);
#ifdef CONFIG_ENCODERS
    register_avcodec(&amr_wb_encoder);
#endif //CONFIG_ENCODERS
#endif /* AMR_WB */

    /* pcm codecs */

#ifdef CONFIG_ENCODERS
#define PCM_CODEC(id, name) \
    register_avcodec(& name ## _encoder); \
    register_avcodec(& name ## _decoder); \

#else
#define PCM_CODEC(id, name) \
    register_avcodec(& name ## _decoder);
#endif

PCM_CODEC(CODEC_ID_PCM_S16LE, pcm_s16le);
PCM_CODEC(CODEC_ID_PCM_S16BE, pcm_s16be);
PCM_CODEC(CODEC_ID_PCM_U16LE, pcm_u16le);
PCM_CODEC(CODEC_ID_PCM_U16BE, pcm_u16be);
PCM_CODEC(CODEC_ID_PCM_S8, pcm_s8);
PCM_CODEC(CODEC_ID_PCM_U8, pcm_u8);
PCM_CODEC(CODEC_ID_PCM_ALAW, pcm_alaw);
PCM_CODEC(CODEC_ID_PCM_MULAW, pcm_mulaw);

    /* adpcm codecs */
PCM_CODEC(CODEC_ID_ADPCM_IMA_QT, adpcm_ima_qt);
PCM_CODEC(CODEC_ID_ADPCM_IMA_WAV, adpcm_ima_wav);
PCM_CODEC(CODEC_ID_ADPCM_IMA_DK3, adpcm_ima_dk3);
PCM_CODEC(CODEC_ID_ADPCM_IMA_DK4, adpcm_ima_dk4);
PCM_CODEC(CODEC_ID_ADPCM_IMA_WS, adpcm_ima_ws);
PCM_CODEC(CODEC_ID_ADPCM_IMA_SMJPEG, adpcm_ima_smjpeg);
PCM_CODEC(CODEC_ID_ADPCM_MS, adpcm_ms);
PCM_CODEC(CODEC_ID_ADPCM_4XM, adpcm_4xm);
PCM_CODEC(CODEC_ID_ADPCM_XA, adpcm_xa);
PCM_CODEC(CODEC_ID_ADPCM_ADX, adpcm_adx);
PCM_CODEC(CODEC_ID_ADPCM_EA, adpcm_ea);
PCM_CODEC(CODEC_ID_ADPCM_G726, adpcm_g726);

#undef PCM_CODEC

    /* parsers */ 
    av_register_codec_parser(&mpegvideo_parser);
    av_register_codec_parser(&mpeg4video_parser);
    av_register_codec_parser(&h261_parser);
    av_register_codec_parser(&h263_parser);
    av_register_codec_parser(&h264_parser);

/*     av_register_codec_parser(&mpegaudio_parser); */
#ifdef CONFIG_AC3
    av_register_codec_parser(&ac3_parser);
#endif

#endif

}

static void register_all(void)
{
    static int inited = 0;
    
    if (inited != 0)
	return;
    inited = 1;


    //avcodec_init();
    dsputil_static_init();

    codec_register_all();

#if 0
    mpegps_init();
    mpegts_init();
#ifdef CONFIG_ENCODERS
    crc_init();
    img_init();
#endif //CONFIG_ENCODERS
    raw_init();
/*     mp3_init(); */
    rm_init();
#ifdef CONFIG_RISKY
    asf_init();
#endif
#ifdef CONFIG_ENCODERS
    avienc_init();
#endif //CONFIG_ENCODERS
    avidec_init();
    ff_wav_init();
    swf_init();
    au_init();
#ifdef CONFIG_ENCODERS
    gif_init();
#endif //CONFIG_ENCODERS
    mov_init();
#ifdef CONFIG_ENCODERS
    movenc_init();
    jpeg_init();
#endif //CONFIG_ENCODERS
    ff_dv_init();
    fourxm_init();
#ifdef CONFIG_ENCODERS
    flvenc_init();
#endif //CONFIG_ENCODERS
    flvdec_init();
    str_init();
    roq_init();
    ipmovie_init();
    wc3_init();
    westwood_init();
    film_init();
    idcin_init();
    //flic_init();
    vmd_init();

#if defined(AMR_NB) || defined(AMR_NB_FIXED) || defined(AMR_WB)
    amr_init();
#endif
    yuv4mpeg_init();
    
#ifdef CONFIG_VORBIS
    ogg_init();
#endif

#ifndef CONFIG_WIN32
    ffm_init();
#endif
#ifdef CONFIG_VIDEO4LINUX
    video_grab_init();
#endif
#if defined(CONFIG_AUDIO_OSS) || defined(CONFIG_AUDIO_BEOS)
    audio_init();
#endif

#ifdef CONFIG_DV1394
    dv1394_init();
#endif

    nut_init();
    matroska_init();

#ifdef CONFIG_ENCODERS
    /* image formats */
    av_register_image_format(&pnm_image_format);
    av_register_image_format(&pbm_image_format);
    av_register_image_format(&pgm_image_format);
    av_register_image_format(&ppm_image_format);
    av_register_image_format(&pam_image_format);
    av_register_image_format(&pgmyuv_image_format);
    av_register_image_format(&yuv_image_format);
#ifdef CONFIG_ZLIB
    av_register_image_format(&png_image_format);
#endif
    av_register_image_format(&jpeg_image_format);
    av_register_image_format(&gif_image_format);
    av_register_image_format(&sgi_image_format);
#endif //CONFIG_ENCODERS


    /* file protocols */
    register_protocol(&file_protocol);

#endif // #if 0


#if 0
    register_protocol(&pipe_protocol);
#ifdef CONFIG_NETWORK
    rtsp_init();
    rtp_init();
    register_protocol(&udp_protocol);
    register_protocol(&rtp_protocol);
    register_protocol(&tcp_protocol);
    register_protocol(&http_protocol);
#endif
#endif
}


int codec_main()
{
  printf("codec_misc initializing ...\n");

  register_all();

  return 0;
}

int lef_main()
{
  return 0;
}
