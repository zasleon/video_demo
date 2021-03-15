#include "live_stream.h"

void show_moive()//��ȡ������Ƶ
{
	video_pace=-1;

    // ffmpeg��ر���Ԥ�ȶ��������
	
    
    AVCodec *vCodec						=0;							// ffmpeg������
    //AVFrame *pAVFrame					=0;							// ffmpeg��֡����
    AVFrame *pAVFrameRGB32				=av_frame_alloc();			// ffmpeg��֡����ת����ɫ�ռ��Ļ���
    struct SwsContext *pSwsContext		=0;							// ffmpeg�������ݸ�ʽת��
    AVDictionary *pAVDictionary			=0;							// ffmpeg�����ֵ䣬��������һЩ���������Ե�
    int ret = 0;						// ����ִ�н��
	char buf[1024]={0};					// ����ִ�д���󷵻���Ϣ�Ļ���
    int videoIndex = -1;				// ��Ƶ�����ڵ����
	int numBytes = 0;					// ���������ݳ���
    unsigned char* outBuffer = 0;		// ���������ݴ�Ż�����
	SYSTEMTIME startTime,nextTime;		// ����ǰʱ��//�������ʱ��
	int VideoTimeStamp=0;				// ʱ�������ֹ��Ƶû���Դ���pts

    if(!pAVFormatContext || !pAVFrameRGB32){cout << "Failed to alloc";return;}

    // ����һ��ע�����������ͱ��������Ҳ����ֻע��һ�࣬��ע��������ע��������ȣ�
    av_register_all();
    avformat_network_init();
    // ����������ļ�(ffmpeg�ɹ��򷵻�0)
    cout << "��:" << rtspUrl<<endl;
    if(avformat_open_input(&pAVFormatContext,rtspUrl, NULL, &pAVDictionary)){av_strerror(ret, buf, 1024);cout<<"��ʧ��,������:"<<ret;return;}
	// ��������̽����ý����Ϣ
    if(avformat_find_stream_info(pAVFormatContext, 0)< 0){cout << "avformat_find_stream_infoʧ��\n";return;}
    
    /*ԭ�����ϴ���
	// �����ģ���ȡ����Ϣ,��ȡ��Ƶ��Ϣ
	for(int index = 0; index < pAVFormatContext->nb_streams; index++)
    { 
        switch (pAVFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_UNKNOWN"<<endl;break;
			case AVMEDIA_TYPE_VIDEO:videoIndex = index;	cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_VIDEO"<<endl;break;
			case AVMEDIA_TYPE_AUDIO:AudioIndex = index;	cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_AUDIO"<<endl;break;
			case AVMEDIA_TYPE_DATA:						cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_DATA"<<endl;break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_SUBTITLE"<<endl;break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_ATTACHMENT"<<endl;break;
			case AVMEDIA_TYPE_NB:						cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_NB"<<endl;break;
			default:break;
        }
        if(videoIndex != -1)// �Ѿ��ҵ���ƵƷ��,����ѭ��
		{
			pVideoCodecCtx = pAVFormatContext->streams[index]->codec;
			pVStream = pAVFormatContext->streams[index];break;}
    }//��ȡ����

	VideoIndex=videoIndex;
	if(videoIndex == -1 || !pVideoCodecCtx)
	{
		cout << "Failed to find video stream";
		if(AudioIndex<0)return;
		else//����������Ƶ
		{
			if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//��ʼ��SDL 
				{cout<<"Failed SDL_Init\n";				return;}
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//��ʼ��Ƶ�̲߳��𲢲���
			SDL_Delay(1000);
			while(!output_over){Sleep(1000);}
			SDL_Quit();
		}
	}//�ж��Ƿ��ҵ���Ƶ��
	

    // �����壺���ҵ�����Ƶ��Ѱ������
    vCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
    if(!vCodec){cout<<"Failed to avcodec_find_decoder(pVideoCodecCtx->codec_id):"<< pVideoCodecCtx->codec_id;return; }
	*/

	videoIndex=av_find_best_stream(pAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &vCodec, NULL);// �����ģ���ȡ����Ϣ,��ȡ��Ƶ��Ϣ
	if(ret<0)//û����Ƶ����ѯ�Ƿ�����Ƶ
	{
		cout<<"��Ƶ��ʧ��!\n";
		if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//��ʼ��SDL 
		{cout<<"Failed SDL_Init\n";				return;}
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//��ʼ��Ƶ�̲߳��𲢲���
		SDL_Delay(1000);
		while(!audio_output_over){Sleep(1000);}
		SDL_Quit();
	}

	// �����壺���ҵ�����Ƶ��Ѱ������
	pVideoCodecCtx=avcodec_alloc_context3(vCodec);
	avcodec_parameters_to_context(pVideoCodecCtx,pAVFormatContext->streams[videoIndex]->codecpar);
	pVStream = pAVFormatContext->streams[videoIndex];
	pVideoCodecCtx->pkt_timebase=pAVFormatContext->streams[videoIndex]->time_base;

    // ���������򿪽�����    
	av_dict_set(&pAVDictionary, "probesize", "4", 0);
    av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);// ���û����С 1024000byte    
    av_dict_set(&pAVDictionary, "stimeout", "2000000", 0);// ���ó�ʱʱ�� 20s    20000000
    av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);// ���������ʱ 3s,30000000    
    av_dict_set(&pAVDictionary, "rtsp_transport", "udp", 0);// ���ô򿪷�ʽ tcp/udp
	
    if(avcodec_open2(pVideoCodecCtx, vCodec, &pAVDictionary)){cout << "avcodec_open2 failed\n";return;}

    // ��ʾ��Ƶ��صĲ�����Ϣ�����������ģ�                            
    cout<<"������:"		<<pVideoCodecCtx->bit_rate			<<endl;
    cout<<"��ʱ��:"		<<pVStream->duration* av_q2d(pVStream->time_base)<<"��("	<<float(pVStream->duration* av_q2d(pVStream->time_base)/60.0)<<"����)\n";
    cout<<"��֡��:"		<<pVStream->nb_frames			<<endl;	
    cout<<"��ʽ:"		<<pVideoCodecCtx->pix_fmt			<<endl;//��ʽΪ0˵����AV_PIX_FMT_YUV420P
    cout<<"��:"			<<pVideoCodecCtx->width				<<"\t�ߣ�"		<<pVideoCodecCtx->height				<<endl;
	cout<<"�ļ���ĸ:"	<<pVideoCodecCtx->time_base.den		<<"\t�ļ�����:"	<<pVideoCodecCtx->time_base.num		<<endl;
    cout<<"֡�ʷ�ĸ:"	<<pVStream->avg_frame_rate.den	<<"\t֡�ʷ���:"	<<pVStream->avg_frame_rate.num	<<endl;
    // ����ʱ����ʱ�����֡�ʣ���Ϊ׼ȷ��
	double fps=pVStream->avg_frame_rate.num * 1.0f / pVStream->avg_frame_rate.den;if(fps<=0){cout<<fps<<endl;return;}//֡��
    double interval = 1 * 1000 / fps;//֡���
    cout<<"ƽ��֡��:"	<< fps<<endl;
    cout<<"֡���:"		<< interval << "ms"<<endl;


	// �����ߣ����õ���ԭʼ���ݸ�ʽ��������ת��Ϊָ���ĸ�ʽ�߿��С  AV_PIX_FMT_YUV420P        pVideoCodecCtx->pix_fmt AV_PIX_FMT_RGBA
	pSwsContext = sws_getContext(pVideoCodecCtx->width,pVideoCodecCtx->height,pVideoCodecCtx->pix_fmt,pVideoCodecCtx->width,pVideoCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_FAST_BILINEAR,0,0,0);
	numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,pVideoCodecCtx->width,pVideoCodecCtx->height);
	outBuffer = (unsigned char *)av_malloc(numBytes);
	avpicture_fill((AVPicture *)pAVFrameRGB32,outBuffer,AV_PIX_FMT_YUV420P,pVideoCodecCtx->width,pVideoCodecCtx->height);//pAVFrame32��dataָ��ָ����outBuffer
		
	//---------------------------------------------------------SDL��ر���Ԥ�ȶ���----------------------------------------------------------------
	if(pVideoCodecCtx->width>my_width)set_width=my_width;else set_width=pVideoCodecCtx->width;
	if(pVideoCodecCtx->height>my_height) set_height=my_height;else set_height=pVideoCodecCtx->height;

    if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER)){cout<<"Failed SDL_Init\n";return;}//��ʼ��SDL 
	SDL_Window *pSDLWindow = 0;
	SDL_Renderer *pSDLRenderer = 0;
	SDL_Surface *pSDLSurface = 0;
	SDL_Texture *pSDLTexture = 0;
	SDL_Event event;
		pSDLWindow = SDL_CreateWindow("ZasLeonPlayer",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,set_width,set_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);//������ʾ���С
	if(!pSDLWindow)
		{cout<<"Failed SDL_CreateWindow\n";		return;}
	pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
	if(!pSDLRenderer)
		{cout<<"Failed SDL_CreateRenderer\n";	return;}
	pSDLTexture = SDL_CreateTexture(pSDLRenderer,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,pVideoCodecCtx->width,pVideoCodecCtx->height);
	if(!pSDLTexture)
		{cout<<"Failed SDL_CreateTexture\n";	return;}
	//---------------------------------------------------------SDL�������------------------------------------------------------------------------

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//��ʼ��Ƶ�̲߳��𲢲���

	AVPacket *pVVPacket=av_packet_alloc();

		while(av_read_frame(pAVFormatContext, pVVPacket) >= 0)
		{
			//GetLocalTime(&startTime);//��ȡ����ǰ��ȷʱ��
			if(pVVPacket->stream_index == videoIndex)//�����֡Ϊ��Ƶ֡����Ƶ����ʼ
			{
				ret=avcodec_send_packet(pVideoCodecCtx, pVVPacket);
				if(ret){cout<<"avcodec_send_packetʧ��!,������"<<ret;break;}
				AVFrame *pAVFrame=av_frame_alloc();
				while(!avcodec_receive_frame(pVideoCodecCtx, pAVFrame))
				{
					sws_scale(pSwsContext,(const uint8_t * const *)pAVFrame->data,pAVFrame->linesize,0,pVideoCodecCtx->height,pAVFrameRGB32->data,pAVFrameRGB32->linesize);

					SDL_UpdateYUVTexture(pSDLTexture,NULL,pAVFrame->data[0], pAVFrame->linesize[0],pAVFrame->data[1], pAVFrame->linesize[1],pAVFrame->data[2], pAVFrame->linesize[2]);

					SDL_RenderClear(pSDLRenderer);
					// Texture���Ƶ�Renderer
					SDL_Rect sdlRect;
					sdlRect.x=0;sdlRect.y=0;
					if(pAVFrame->width>my_width)sdlRect.w=my_width;else sdlRect.w=pAVFrame->width;//������ʾ���С
					if(pAVFrame->height>my_height)sdlRect.h = my_height;else sdlRect.h = pAVFrame->height;
				
					SDL_RenderCopy(pSDLRenderer, pSDLTexture, 0, &sdlRect) ;
					SDL_RenderPresent(pSDLRenderer);// ����Renderer��ʾ
					SDL_PollEvent(&event);// �¼�����
					//cout<<pAVFrame->pts<<"   "<<av_q2d(pVStream->time_base)<<endl;
					if(pAVFrame->pts>0)//��ЩӰƬ����֡����pts
						video_pace=pAVFrame->pts*av_q2d(pVStream->time_base);//���¸�֡��ʱ�䣬ʵ������Ƶͬ��
					else
						video_pace+=interval/float(1000);
				}
				while(video_pace>audio_pace&&!audio_output_over){}//��ֹ������ͬ��������ѭ���ȴ���ͬ��.����������������ġ�ԭʵ�ֲ����ӳٴ��롱
				if(audio_output_over){Sleep(int(interval/float(1000)));cout<<"s"<<int(interval)<<endl;}//�����Ƶ�����˵������滹û���꣬��֡���ʱ�������
		

			}//һ֡��Ƶ�������

		}//��Ƶ���볹����ɣ���ѭ������
		
	cout<<"��Ƶ�������\n";
	write_video_buff_over=true;
	while(!audio_output_over||!video_output_over){Sleep(30);}//�����Ƶû����꣬���еȴ�
	cout << "�ͷŻ�����Դ,�ر�SDL"<<endl;
	Sleep(500);
	if(pVVPacket){av_free_packet(pVVPacket);pVVPacket = 0;}
	if(outBuffer){av_free(outBuffer);outBuffer = 0;}
	if(pSwsContext){sws_freeContext(pSwsContext);pSwsContext = 0;}
	if(pAVFrameRGB32){av_frame_free(&pAVFrameRGB32);pAVFrameRGB32 = 0;}
    if(pVideoCodecCtx){avcodec_close(pVideoCodecCtx);pVideoCodecCtx = 0;}
    if(pAVFormatContext){avformat_close_input(&pAVFormatContext);avformat_free_context(pAVFormatContext);pAVFormatContext = 0;}
    SDL_DestroyRenderer(pSDLRenderer);//������Ⱦ��
    SDL_DestroyWindow(pSDLWindow);cout<<"����ִ��SDL_Quit()\n";// ���ٴ���
	SDL_Quit(); //�˳�SDL
	
}
		/*
			if(false)//�ƶ���ָ��ʱ���
			{
				cout<<"ǿ��\n";
				int set_time=audio_pace;change=true;//��1��
				auto time_base= pAVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pAVFormatContext->streams[videoIndex]->start_time + av_rescale(set_time, time_base.den, time_base.num);
				ret = av_seek_frame(pAVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD );
				continue;
			}*/

/*//ԭʵ�ֲ����ӳٴ���
if(audio_over||video_pace>audio_pace)
			{
				GetLocalTime(&nextTime);//��ȡ�����ȷʱ��
				if(nextTime.wMilliseconds<startTime.wMilliseconds)nextTime.wMilliseconds=nextTime.wMilliseconds+1000;//�����������С�ڽ���ǰ�������ʱ��+1000���벹
				int delaytime=int(interval)-int(nextTime.wMilliseconds-startTime.wMilliseconds);//�������ǰ������
				if(delaytime<interval&&delaytime>0)Sleep(delaytime);//�����ӳ٣���ֹ�����ٶȹ��죬�������delaytime-1�û����һ��ˢ�£����������ѭ������ͬ��
			}
			else//�������������
			
			if(video_pace<audio_pace-5)//���������Ƶ̫Զ���л����û���ؼ�֡
			{
				auto time_base= pAVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pAVFormatContext->streams[videoIndex]->start_time + av_rescale(int(audio_pace), time_base.den, time_base.num);
				ret = av_seek_frame(pAVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD);
			 }*/