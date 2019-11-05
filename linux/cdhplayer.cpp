#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

#ifdef __MINGW32__
#undef main //防止SDL的MAIN问题
#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL; 
  AVPacket        packet;
  int             frameFinished;

  AVDictionary    *optionsDict = NULL;
  struct SwsContext *sws_ctx = NULL;

  SDL_Overlay     *bmp = NULL;
  SDL_Surface     *screen_sdl = NULL;
  SDL_Rect        rect;
  SDL_Event       event;

  if(argc < 2) {
    fprintf(stderr, "Usage: please input <file>\n");
    exit(1);
  }

  //初始化所有组件,只有调用了该函数,才能使用复用器和编解码器
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  //打开一个文件并解析。可解析的内容包括：视频流、音频流、视频流参数、音频流参数、视频帧索引。
  //该函数读取文件的头信息，并将其信息保存到AVFormatContext结构体中
  if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
    return -1; // Couldn't open file
  
  //作用是为pFormatContext->streams填充上正确的音视频格式信息,通过av_dump_format函数输出
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  //将音视频数据格式通过av_log输出到指定的文件或者控制台,删除该函数的调用没有任何的影响
  av_dump_format(pFormatCtx, 0, argv[1], 0);
  
  //要解码视频，首先在AVFormatContext包含的多个流中找到CODEC，类型为AVMEDIA_TYPE_VIDEO
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // 找不到就结束
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // 寻找解码器
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // 找不到Codec
  }
  
  // 调用avcodec_open2打开codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; // 无法打开codec
  
  // 对 video frame进行分配空间
  pFrame=av_frame_alloc();

  //使用SDL做界面
  screen_sdl = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
  if(!screen_sdl ) {
    fprintf(stderr, "SDL: could not set video mode - exiting\n");
    exit(1);
  }
  
  // 把YUV数据放到screen
  bmp = SDL_CreateYUVOverlay(pCodecCtx->width,
				 pCodecCtx->height,
				 SDL_YV12_OVERLAY,
				 screen_sdl );
  
  sws_ctx =
    sws_getContext
    (
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,	//定义输入图像信息（寬、高、颜色空间(像素格式)）
        pCodecCtx->width,
        pCodecCtx->height,	
        AV_PIX_FMT_YUV420P,//定义输出图像信息(寬、高、颜色空间(像素格式))
        SWS_BILINEAR,//选择缩放算法（只有当输入输出图像大小不同时有效)
        NULL,
        NULL,
        NULL
    );

  // 读取frames数据并且保存
  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    if(packet.stream_index==videoStream) {
      // Decode video frame
      //作用是解码一帧视频数据。输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			   &packet);
      
      if(frameFinished) {
	SDL_LockYUVOverlay(bmp);
//把图片转为YUV使用的格式
	AVPicture pict;
	pict.data[0] = bmp->pixels[0];
	pict.data[1] = bmp->pixels[2];
	pict.data[2] = bmp->pixels[1];

	pict.linesize[0] = bmp->pitches[0];
	pict.linesize[1] = bmp->pitches[2];
	pict.linesize[2] = bmp->pitches[1];

    sws_scale
    (
        sws_ctx, 
        (uint8_t const * const *)pFrame->data, 
        pFrame->linesize, 
        0,
        pCodecCtx->height,
        pict.data,
        pict.linesize
    );
	
	SDL_UnlockYUVOverlay(bmp);
	
	rect.x = 0;
	rect.y = 0;
	rect.w = pCodecCtx->width;
	rect.h = pCodecCtx->height;
	SDL_DisplayYUVOverlay(bmp, &rect);
      
      }
    }
    
    //释放 packet
    av_free_packet(&packet);
    SDL_PollEvent(&event);
    switch(event.type) {
    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    default:
      break;
    }

  }
  
  // 释放掉frame
  av_free(pFrame);
  //关闭打开的流和解码器
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);
  
  return 0;
}


