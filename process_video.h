#include "live_stream.h"

void show_moive()//读取本地视频
{
	video_pace=-1;

    // ffmpeg相关变量预先定义与分配
	
    
    AVCodec *vCodec						=0;							// ffmpeg编码器
    //AVFrame *pAVFrame					=0;							// ffmpeg单帧缓存
    AVFrame *pAVFrameRGB32				=av_frame_alloc();			// ffmpeg单帧缓存转换颜色空间后的缓存
    struct SwsContext *pSwsContext		=0;							// ffmpeg编码数据格式转换
    AVDictionary *pAVDictionary			=0;							// ffmpeg数据字典，用于配置一些编码器属性等
    int ret = 0;						// 函数执行结果
	char buf[1024]={0};					// 函数执行错误后返回信息的缓存
    int videoIndex = -1;				// 视频流所在的序号
	int numBytes = 0;					// 解码后的数据长度
    unsigned char* outBuffer = 0;		// 解码后的数据存放缓存区
	SYSTEMTIME startTime,nextTime;		// 解码前时间//解码完成时间
	int VideoTimeStamp=0;				// 时间戳，防止视频没有自带的pts

    if(!pAVFormatContext || !pAVFrameRGB32){cout << "Failed to alloc";return;}

    // 步骤一：注册所有容器和编解码器（也可以只注册一类，如注册容器、注册编码器等）
    av_register_all();
    avformat_network_init();
    // 步骤二：打开文件(ffmpeg成功则返回0)
    cout << "打开:" << rtspUrl<<endl;
    if(avformat_open_input(&pAVFormatContext,rtspUrl, NULL, &pAVDictionary)){av_strerror(ret, buf, 1024);cout<<"打开失败,错误编号:"<<ret;return;}
	// 步骤三：探测流媒体信息
    if(avformat_find_stream_info(pAVFormatContext, 0)< 0){cout << "avformat_find_stream_info失败\n";return;}
    
    /*原来的老代码
	// 步骤四：提取流信息,提取视频信息
	for(int index = 0; index < pAVFormatContext->nb_streams; index++)
    { 
        switch (pAVFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_UNKNOWN"<<endl;break;
			case AVMEDIA_TYPE_VIDEO:videoIndex = index;	cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_VIDEO"<<endl;break;
			case AVMEDIA_TYPE_AUDIO:AudioIndex = index;	cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_AUDIO"<<endl;break;
			case AVMEDIA_TYPE_DATA:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_DATA"<<endl;break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_SUBTITLE"<<endl;break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_ATTACHMENT"<<endl;break;
			case AVMEDIA_TYPE_NB:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_NB"<<endl;break;
			default:break;
        }
        if(videoIndex != -1)// 已经找到视频品流,跳出循环
		{
			pVideoCodecCtx = pAVFormatContext->streams[index]->codec;
			pVStream = pAVFormatContext->streams[index];break;}
    }//提取结束

	VideoIndex=videoIndex;
	if(videoIndex == -1 || !pVideoCodecCtx)
	{
		cout << "Failed to find video stream";
		if(AudioIndex<0)return;
		else//单纯播放音频
		{
			if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//初始化SDL 
				{cout<<"Failed SDL_Init\n";				return;}
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//开始音频线程部署并播放
			SDL_Delay(1000);
			while(!output_over){Sleep(1000);}
			SDL_Quit();
		}
	}//判断是否找到视频流
	

    // 步骤五：对找到的视频流寻解码器
    vCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
    if(!vCodec){cout<<"Failed to avcodec_find_decoder(pVideoCodecCtx->codec_id):"<< pVideoCodecCtx->codec_id;return; }
	*/

	videoIndex=av_find_best_stream(pAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &vCodec, NULL);// 步骤四：提取流信息,提取视频信息
	if(ret<0)//没有视频，查询是否有音频
	{
		cout<<"视频打开失败!\n";
		if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//初始化SDL 
		{cout<<"Failed SDL_Init\n";				return;}
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//开始音频线程部署并播放
		SDL_Delay(1000);
		while(!audio_output_over){Sleep(1000);}
		SDL_Quit();
	}

	// 步骤五：对找到的视频流寻解码器
	pVideoCodecCtx=avcodec_alloc_context3(vCodec);
	avcodec_parameters_to_context(pVideoCodecCtx,pAVFormatContext->streams[videoIndex]->codecpar);
	pVStream = pAVFormatContext->streams[videoIndex];
	pVideoCodecCtx->pkt_timebase=pAVFormatContext->streams[videoIndex]->time_base;

    // 步骤六：打开解码器    
	av_dict_set(&pAVDictionary, "probesize", "4", 0);
    av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);// 设置缓存大小 1024000byte    
    av_dict_set(&pAVDictionary, "stimeout", "2000000", 0);// 设置超时时间 20s    20000000
    av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);// 设置最大延时 3s,30000000    
    av_dict_set(&pAVDictionary, "rtsp_transport", "udp", 0);// 设置打开方式 tcp/udp
	
    if(avcodec_open2(pVideoCodecCtx, vCodec, &pAVDictionary)){cout << "avcodec_open2 failed\n";return;}

    // 显示视频相关的参数信息（编码上下文）                            
    cout<<"比特率:"		<<pVideoCodecCtx->bit_rate			<<endl;
    cout<<"总时长:"		<<pVStream->duration* av_q2d(pVStream->time_base)<<"秒("	<<float(pVStream->duration* av_q2d(pVStream->time_base)/60.0)<<"分钟)\n";
    cout<<"总帧数:"		<<pVStream->nb_frames			<<endl;	
    cout<<"格式:"		<<pVideoCodecCtx->pix_fmt			<<endl;//格式为0说明是AV_PIX_FMT_YUV420P
    cout<<"宽:"			<<pVideoCodecCtx->width				<<"\t高："		<<pVideoCodecCtx->height				<<endl;
	cout<<"文件分母:"	<<pVideoCodecCtx->time_base.den		<<"\t文件分子:"	<<pVideoCodecCtx->time_base.num		<<endl;
    cout<<"帧率分母:"	<<pVStream->avg_frame_rate.den	<<"\t帧率分子:"	<<pVStream->avg_frame_rate.num	<<endl;
    // 有总时长的时候计算帧率（较为准确）
	double fps=pVStream->avg_frame_rate.num * 1.0f / pVStream->avg_frame_rate.den;if(fps<=0){cout<<fps<<endl;return;}//帧率
    double interval = 1 * 1000 / fps;//帧间隔
    cout<<"平均帧率:"	<< fps<<endl;
    cout<<"帧间隔:"		<< interval << "ms"<<endl;


	// 步骤七：对拿到的原始数据格式进行缩放转换为指定的格式高宽大小  AV_PIX_FMT_YUV420P        pVideoCodecCtx->pix_fmt AV_PIX_FMT_RGBA
	pSwsContext = sws_getContext(pVideoCodecCtx->width,pVideoCodecCtx->height,pVideoCodecCtx->pix_fmt,pVideoCodecCtx->width,pVideoCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_FAST_BILINEAR,0,0,0);
	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,pVideoCodecCtx->width,pVideoCodecCtx->height);
	outBuffer = (unsigned char *)av_malloc(numBytes);
	avpicture_fill((AVPicture *)pAVFrameRGB32,outBuffer,AV_PIX_FMT_YUV420P,pVideoCodecCtx->width,pVideoCodecCtx->height);//pAVFrame32的data指针指向了outBuffer
		
	//---------------------------------------------------------SDL相关变量预先定义----------------------------------------------------------------
	if(pVideoCodecCtx->width>my_width)set_width=my_width;else set_width=pVideoCodecCtx->width;
	if(pVideoCodecCtx->height>my_height) set_height=my_height;else set_height=pVideoCodecCtx->height;

    if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER)){cout<<"Failed SDL_Init\n";return;}//初始化SDL 
	SDL_Window *pSDLWindow = 0;
	SDL_Renderer *pSDLRenderer = 0;
	SDL_Surface *pSDLSurface = 0;
	SDL_Texture *pSDLTexture = 0;
	SDL_Event event;
		pSDLWindow = SDL_CreateWindow("ZasLeonPlayer",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,set_width,set_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);//设置显示框大小
	if(!pSDLWindow)
		{cout<<"Failed SDL_CreateWindow\n";		return;}
	pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
	if(!pSDLRenderer)
		{cout<<"Failed SDL_CreateRenderer\n";	return;}
	pSDLTexture = SDL_CreateTexture(pSDLRenderer,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,pVideoCodecCtx->width,pVideoCodecCtx->height);
	if(!pSDLTexture)
		{cout<<"Failed SDL_CreateTexture\n";	return;}
	//---------------------------------------------------------SDL定义结束------------------------------------------------------------------------

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//开始音频线程部署并播放

	AVPacket *pVVPacket=av_packet_alloc();

		while(av_read_frame(pAVFormatContext, pVVPacket) >= 0)
		{
			//GetLocalTime(&startTime);//获取解码前精确时间
			if(pVVPacket->stream_index == videoIndex)//如果该帧为视频帧，视频处理开始
			{
				ret=avcodec_send_packet(pVideoCodecCtx, pVVPacket);
				if(ret){cout<<"avcodec_send_packet失败!,错误码"<<ret;break;}
				AVFrame *pAVFrame=av_frame_alloc();
				while(!avcodec_receive_frame(pVideoCodecCtx, pAVFrame))
				{
					sws_scale(pSwsContext,(const uint8_t * const *)pAVFrame->data,pAVFrame->linesize,0,pVideoCodecCtx->height,pAVFrameRGB32->data,pAVFrameRGB32->linesize);

					SDL_UpdateYUVTexture(pSDLTexture,NULL,pAVFrame->data[0], pAVFrame->linesize[0],pAVFrame->data[1], pAVFrame->linesize[1],pAVFrame->data[2], pAVFrame->linesize[2]);

					SDL_RenderClear(pSDLRenderer);
					// Texture复制到Renderer
					SDL_Rect sdlRect;
					sdlRect.x=0;sdlRect.y=0;
					if(pAVFrame->width>my_width)sdlRect.w=my_width;else sdlRect.w=pAVFrame->width;//更新显示框大小
					if(pAVFrame->height>my_height)sdlRect.h = my_height;else sdlRect.h = pAVFrame->height;
				
					SDL_RenderCopy(pSDLRenderer, pSDLTexture, 0, &sdlRect) ;
					SDL_RenderPresent(pSDLRenderer);// 更新Renderer显示
					SDL_PollEvent(&event);// 事件处理
					//cout<<pAVFrame->pts<<"   "<<av_q2d(pVStream->time_base)<<endl;
					if(pAVFrame->pts>0)//有些影片无总帧数无pts
						video_pace=pAVFrame->pts*av_q2d(pVStream->time_base);//更新该帧的时间，实现音视频同步
					else
						video_pace+=interval/float(1000);
				}
				while(video_pace>audio_pace&&!audio_output_over){}//防止话音不同步，靠死循环等待来同步.这条语句代替了上面的“原实现播放延迟代码”
				if(audio_output_over){Sleep(int(interval/float(1000)));cout<<"s"<<int(interval)<<endl;}//如果音频播完了但还画面还没播完，按帧间隔时间继续播
		

			}//一帧视频处理结束

		}//视频解码彻底完成，死循环结束
		
	cout<<"视频解码结束\n";
	write_video_buff_over=true;
	while(!audio_output_over||!video_output_over){Sleep(30);}//如果音频没输出完，进行等待
	cout << "释放回收资源,关闭SDL"<<endl;
	Sleep(500);
	if(pVVPacket){av_free_packet(pVVPacket);pVVPacket = 0;}
	if(outBuffer){av_free(outBuffer);outBuffer = 0;}
	if(pSwsContext){sws_freeContext(pSwsContext);pSwsContext = 0;}
	if(pAVFrameRGB32){av_frame_free(&pAVFrameRGB32);pAVFrameRGB32 = 0;}
    if(pVideoCodecCtx){avcodec_close(pVideoCodecCtx);pVideoCodecCtx = 0;}
    if(pAVFormatContext){avformat_close_input(&pAVFormatContext);avformat_free_context(pAVFormatContext);pAVFormatContext = 0;}
    SDL_DestroyRenderer(pSDLRenderer);//销毁渲染器
    SDL_DestroyWindow(pSDLWindow);cout<<"即将执行SDL_Quit()\n";// 销毁窗口
	SDL_Quit(); //退出SDL
	
}
		/*
			if(false)//移动到指定时间点
			{
				cout<<"强切\n";
				int set_time=audio_pace;change=true;//第1秒
				auto time_base= pAVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pAVFormatContext->streams[videoIndex]->start_time + av_rescale(set_time, time_base.den, time_base.num);
				ret = av_seek_frame(pAVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD );
				continue;
			}*/

/*//原实现播放延迟代码
if(audio_over||video_pace>audio_pace)
			{
				GetLocalTime(&nextTime);//获取解码后精确时间
				if(nextTime.wMilliseconds<startTime.wMilliseconds)nextTime.wMilliseconds=nextTime.wMilliseconds+1000;//如果解码后毫秒小于解码前，解码后时间+1000毫秒补
				int delaytime=int(interval)-int(nextTime.wMilliseconds-startTime.wMilliseconds);//计算解码前后毫秒差
				if(delaytime<interval&&delaytime>0)Sleep(delaytime);//正常延迟，防止播放速度过快，这里故意delaytime-1让画面快一点刷新，靠下面的死循环补正同步
			}
			else//画面比声音慢了
			
			if(video_pace<audio_pace-5)//如果距离音频太远，切换到该画面关键帧
			{
				auto time_base= pAVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pAVFormatContext->streams[videoIndex]->start_time + av_rescale(int(audio_pace), time_base.den, time_base.num);
				ret = av_seek_frame(pAVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD);
			 }*/