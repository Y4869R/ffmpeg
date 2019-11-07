#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <SDL2/SDL.h>
};

#define LOG_INFO(format, ...) {  fprintf(stderr, "\033[42m### CDH DEBUG ###\033[0m\033[032m " format " \033[0m \033[42m###\033[0m\n", ##__VA_ARGS__ ); }
#define LOG_ERROR(format, ...) {  fprintf(stderr, "\033[41m### CDH DEBUG ###\033[0m\033[031m " format " \033[0m \033[41m###\033[0m\n", ##__VA_ARGS__ ); }

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit=0;
int thread_pause=0;

int sfp_refresh_thread(void *opaque){
  thread_exit=0;
  thread_pause=0;

  while (!thread_exit) {
    if(!thread_pause){
      SDL_Event event;
      event.type = SFM_REFRESH_EVENT;
      SDL_PushEvent(&event);
    }
    SDL_Delay(40);
  }
  thread_exit=0;
  thread_pause=0;
  //Break
  SDL_Event event;
  event.type = SFM_BREAK_EVENT;
  SDL_PushEvent(&event);

  return 0;
}


int main(int argc, char* argv[])
{

  AVFormatContext *pFormatCtx;
  int             i, videoindex;
  AVCodecContext  *pCodecCtx;
  AVCodec         *pCodec;
  AVFrame *pFrame;				//由包解码得到的原始帧
  AVFrame *pFrameYUV;			//由原始帧色彩转换得到
  AVPacket *packet = NULL;		//从流中读出的一段数据包
  unsigned char *out_buffer;
  int ret, got_picture;

  //------------SDL----------------
  int screen_w,screen_h;
  SDL_Window *screen;
  SDL_Renderer* sdlRenderer;
  SDL_Texture* sdlTexture;
  SDL_Rect sdlRect;
  SDL_Thread *video_tid;
  SDL_Event event;

  struct SwsContext *img_convert_ctx;

  //char filepath[]="bigbuckbunny_480x272.h265";
  //char filepath[]="Titanic.ts";
  char *filepath=argv[1];

  //step 0 : av_register_all
  //初始化libavformat，注册所有复用器/解码器
  LOG_INFO("Start!");
  //av_register_all();	//已经过时，直接不再使用
  avformat_network_init();
  pFormatCtx = avformat_alloc_context();

  //step 1 : avformat_open_input
  //打开视频文件，读取文件头，将文件格式信息存储在AVFormatContext对象中
  LOG_INFO("Open stream file : %s !", filepath);
  if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
    printf("Couldn't open input stream.\n");
    return -1;
  }

  //step 2: avformat_find_stream_info
  //搜索流信息，读取一段数据，尝试解码，将取到的流数据填到pFormatCtx->streams
  //_fmt_ctx->streams是一个指针数组，数组大小是pFormatCtx->nb_streams
  LOG_INFO("Find stream info!");
  if(avformat_find_stream_info(pFormatCtx,NULL)<0){
    printf("Couldn't find stream information.\n");
    return -1;
  }

  // 将文件相关信息打印在标准错误设备上
    av_dump_format(p_fmt_ctx, 0, argv[1], 0);

  //step 3: avcodec_find_decoder
  //查找第一个视频流,构建解码器AVCodecContext
  LOG_INFO("Find video decoder!");
  videoindex=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
      videoindex=i;
      break;
    }
  if(videoindex==-1){
    printf("Didn't find a video stream.\n");
    return -1;
  }
  //获取解码器
  pCodecCtx=pFormatCtx->streams[videoindex]->codec;
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL){
    printf("Codec not found.\n");
    return -1;
  }

  //step 4: avcodec_open2
  //初始化解码器
  LOG_INFO("Open codec!");
  if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
    printf("Could not open codec.\n");
    return -1;
  }

  //Output Info-----------------------------
  printf("---------------- File Information ---------------\n");
  av_dump_format(pFormatCtx,0,filepath,0);
  printf("-------------------------------------------------\n");



  //step 5: prepare SDL
  LOG_INFO("Prepare SDL!");
  //step 5.0: SDL_Init
  //初始化SDL子系统：缺省(事件处理、文件IO、线程)、视频、音频、定时器
  LOG_INFO("Initialize SDL!")
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    printf( "Could not initialize SDL - %s\n", SDL_GetError());
    return -1;
  }

  //step 5.1: SDL_CreateWindow
  //创建SDL窗口，SDL 2.0支持多窗口
  LOG_INFO("Create window for display video!");
  //SDL 2.0 Support for multiple windows
  screen_w = pCodecCtx->width;
  screen_h = pCodecCtx->height;
  //创建窗口时，SDL_WINDOW_RESIZABLE表示能缩放窗口大小
  screen = SDL_CreateWindow("title for player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      screen_w, screen_h,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

  if(!screen) {
    printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
    return -1;
  }

  //step 5.2: SDL_CreateRenderer
  //创建SDL_Renderer 渲染器
  LOG_INFO("Create renderer for render texture to window!");
  sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
  //IYUV: Y + U + V  (3 planes)
  //YV12: Y + V + U  (3 planes)

  //step 5.3: SDL_CreateTexture
  //创建SDL_Texture，一个SDL_Texture对应一帧YUV数据
  LOG_INFO("Create testure for display YUV data!");
  sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);

  sdlRect.x=0;
  sdlRect.y=0;
  sdlRect.w=screen_w;
  sdlRect.h=screen_h;

  //step 5.4: SDL_SetWindowTitle
  //可以重新设置标题，不然以SDL_CreateWindow()的第一个参数作为标题
  LOG_INFO("Set player title!");
  SDL_SetWindowTitle(screen, "CDH player SDL2");


  //step 5.5: SDL_CreateThread
  //创建线程来定时刷新
  LOG_INFO("Create thread to refreash window every 40ms!");
  video_tid = SDL_CreateThread(sfp_refresh_thread,NULL,NULL);
  //------------SDL End------------
  //Event Loop

  //step 6: av_read_frame
  //分配AVFrame结构，并不分配data buffer
  LOG_INFO("Read frame!");
  packet=(AVPacket *)av_malloc(sizeof(AVPacket));
  pFrame=av_frame_alloc();
  pFrameYUV=av_frame_alloc();
  //将作为pFrameYUV的视频数据缓冲区
  out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  pCodecCtx->width, pCodecCtx->height,1));
  //使用给定参数设置pFrameYUV->data和pFrameYUV->linesize
  av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,out_buffer,
      AV_PIX_FMT_YUV420P,pCodecCtx->width, pCodecCtx->height,1);

  //初始化SwsContext，用于图像转换
  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
      pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
  for (;;) {
    //Wait
    SDL_WaitEvent(&event);
    if(event.type==SFM_REFRESH_EVENT){
      while(1){
		//读取一个packet的数据
		//对视频而言，一个packet只包含一个frame
		//对音频而言，若是帧长固定的格式则一个packet可包含整数个frame，若是帧长可变的格式则一个packet只包含一个frame
        if(av_read_frame(pFormatCtx, packet)<0)
          thread_exit=1;

        if(packet->stream_index==videoindex)
          break;
      }
      //step 7: avcodec_decode_video2
      //LOG_INFO("Decode video frame in packet and put result in got_picture.");
      ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
      if(ret < 0){
        printf("Decode Error.\n");
        return -1;
      }
      if(got_picture){
        //step 8: put data to  sdlTexture
        //LOG_INFO("Put data to sdlTexture!");
		// plane: 如YUV有Y、U、V三个plane，RGB有R、G、B三个plane
        // slice: 图像中一片连续的行，必须是连续的，顺序由顶部到底部或由底部到顶部
        // stride/pitch: 一行图像所占的字节数，Stride=BytesPerPixel*Width+Padding，注意对齐
        // AVFrame.*data[]: 每个数组元素指向对应plane
        // AVFrame.linesize[]: 每个数组元素表示对应plane中一行图像所占的字节数
        sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
        //SDL---------------------------
		//使用新的YUV像素数据更新SDL_Rect
        SDL_UpdateTexture( sdlTexture, &sdl_rect, pFrameYUV->data[0], pFrameYUV->linesize[0] );
        SDL_RenderClear( sdlRenderer );
        //SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );
		//将部分图像数据SDL_Texture更新到渲染目标
        SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, NULL);

        //step 9: SDL_RenderPresent
        //LOG_INFO("Render texture to window!");
		//执行渲染，更新到屏幕显示
        SDL_RenderPresent( sdlRenderer );
        //SDL End-----------------------
      }
      av_free_packet(packet);
    }else if(event.type==SDL_KEYDOWN){
      //space key for Pause
	  //按空格键就会暂停/播放
      if(event.key.keysym.sym==SDLK_SPACE)
        thread_pause =! thread_pause;
    }else if(event.type==SDL_QUIT){
      thread_exit=1;
    }else if(event.type==SFM_BREAK_EVENT){
      break;
    }

  }

  //step 10: SDL_Quit free av_free avcodec_close avformat_close_input
  LOG_INFO("Quit SDL, free contexts!");
  sws_freeContext(img_convert_ctx);

  SDL_Quit();
  //--------------
  av_frame_free(&pFrameYUV);
  av_frame_free(&pFrame);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);

  return 0;
}
