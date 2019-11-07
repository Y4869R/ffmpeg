#include <iostream>
#if 1
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
//SDL
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
}
#endif

#define LOG_INFO(format, ...) {  fprintf(stderr, "\033[42m### CDH DEBUG ###\033[0m\033[032m " format " \033[0m \033[42m###\033[0m\n", ##__VA_ARGS__ ); }
#define LOG_ERROR(format, ...) {  fprintf(stderr, "\033[41m### CDH DEBUG ###\033[0m\033[031m " format " \033[0m \033[41m###\033[0m\n", ##__VA_ARGS__ ); }

#define SHOW_FULLSCREEN 1

int main(int argc, char* argv[])
{
  AVFormatContext	*pFormatCtx;
  int i, videoindex;
  AVCodecContext	*pCodecCtx;
  AVCodec *pCodec;
  char* filepath=argv[1];
  AVFrame	*pFrame,*pFrameYUV;
  SDL_Surface *screen;
  AVPacket *packet;

  //step 0 : av_register_all
  LOG_INFO("Start!");
  av_register_all();

  avformat_network_init();
  pFormatCtx = avformat_alloc_context();


  //step 1 : avformat_open_input
  LOG_INFO("Open stream file : %s !", filepath);
  if(avformat_open_input(&pFormatCtx, filepath, NULL, NULL)!=0){
    LOG_ERROR("Couldn't open input stream.");
    return -1;
  }

  //step 2: avformat_find_stream_info
  LOG_INFO("Find stream info!");
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
  {
    LOG_ERROR("Couldn't find stream information.");
    return -1;
  }

  //step 3: avcodec_find_decoder
  LOG_INFO("Find video decoder!");
  videoindex = -1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
    {
      videoindex=i;
      break;
    }
  if(videoindex==-1)
  {
    LOG_ERROR("Didn't find a video stream.");
    return -1;
  }
  pCodecCtx = pFormatCtx->streams[videoindex]->codec;
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL)
  {
    LOG_ERROR("Codec not found.");
    return -1;
  }

  //step 4: avcodec_open2
  LOG_INFO("Open codec!");
  if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
  {
    LOG_ERROR("Could not open codec.");
    return -1;
  }

  //step 5: prepare SDL
  LOG_INFO("Prepare SDL!");
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  //uint8_t *out_buffer;
  //out_buffer=(uint8_t *)malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
  //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

  //SDL
  //step 5.0: SDL_Init
  LOG_INFO("Initialize SDL!")
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    LOG_ERROR( "Could not initialize SDL - %s", SDL_GetError());
    return -1;
  }

  //step 5.1: SDL_SetVideoMode
  LOG_INFO("Set video mode, show up according to the source size or fullscreen!");
#if SHOW_FULLSCREEN
  const SDL_VideoInfo * videoInfo = SDL_GetVideoInfo();
  screen = SDL_SetVideoMode(videoInfo->current_w, videoInfo->current_h, 0, SDL_FULLSCREEN);
#else
  screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
#endif
  if(!screen) {
    LOG_ERROR("SDL: could not set video mode - exiting");
    return -1;
  }

  //step 5.2: SDL_CreateYUVOverlay
  LOG_INFO("Create YUV Overlay!");
  SDL_Overlay *bmp;
  bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);

  //step 5.3: SDL_WM_SetCaption
  LOG_INFO("Set player title!");
  SDL_WM_SetCaption("CDH player", NULL);

  //print file infornation
  LOG_INFO("--------------------------------------------- File Information ---------------------------------------------");
  av_dump_format(pFormatCtx,0,filepath,0);
  LOG_INFO("------------------------------------------------------------------------------------------------------------");

  //step 6: av_read_frame
  LOG_INFO("Read frame!");
  int ret, got_picture;
  int y_size = pCodecCtx->width * pCodecCtx->height;
  packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_new_packet(packet, y_size);
  SDL_Rect rect;
  struct SwsContext *img_convert_ctx;
  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
  while(av_read_frame(pFormatCtx, packet)>=0)
  {
    //save frame to packet, and the frame is video frame
    if(packet->stream_index==videoindex)
    {
      //step 7: avcodec_decode_video2
      //LOG_INFO("Decode video frame in packet and put result in got_picture.");
      ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
      if(ret < 0)
      {
        LOG_ERROR("Decode Error.");
        return -1;
      }
      if(got_picture)
      {
        //step 8: put data to pFrameYUV
        //LOG_INFO("Put data to pFrameYUV!");
        SDL_LockYUVOverlay(bmp);

        pFrameYUV->data[0] = bmp->pixels[0];
        pFrameYUV->data[1] = bmp->pixels[2];
        pFrameYUV->data[2] = bmp->pixels[1];
        pFrameYUV->linesize[0] = bmp->pitches[0];
        pFrameYUV->linesize[1] = bmp->pitches[2];
        pFrameYUV->linesize[2] = bmp->pitches[1];
        //ignore invalid data by calling sws_scale()
        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

        SDL_UnlockYUVOverlay(bmp);
        rect.x = 0;
        rect.y = 0;
        rect.w = pCodecCtx->width;
        rect.h = pCodecCtx->height;

        //step 9: SDL_DisplayYUVOverlay
        //LOG_INFO("Display YUV Overlay!");
        SDL_DisplayYUVOverlay(bmp, &rect);
        //Delay 40ms
        SDL_Delay(40);
      }
    }
    av_free_packet(packet);
  }

  //step 10: SDL_Quit free av_free avcodec_close avformat_close_input
  LOG_INFO("Quit SDL, free contexts!");
  SDL_Quit();
  sws_freeContext(img_convert_ctx);

  //free(out_buffer);
  av_free(pFrameYUV);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);

  return 0;
}

