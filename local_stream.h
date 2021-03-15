#include "vod_stream.h"
//---------------------------------------------------����Ƶ������߼�����----------------------------
void audio_callback(void *userdata, Uint8 *stream, int len) //��Ƶ�ص�������SDL_OpenAudioʹ�ú������ѭ��ʹ�øú�����ֱ��ʹ��SDL_closeAudio�������close��SDL_Quit��������
{
	
	if(audio_output_over&&read_this_buf->bufsize==0)return;//�������������,����Ƶ��������,����

	while(read_this_buf->bufsize<=0)Sleep(100);//����û���Ϊ�գ��ȴ�

	audio_pace=read_this_buf->pts+double(read_this_buf->gap)*(double(read_this_buf->read_buf_index)/double(read_this_buf->bufsize));////������Ƶ���ŵ���ʱ��

	//memcpy(stream, audiobuf[read_count]+audio_buf_read_index[read_count], len);//���Լ�Я����Ϣ���в���,��˵memcpy����ֱ���������ʧ�棬�������������SDL_memset,SDL_MixAudio
	SDL_memset(stream, 0, len);
	if(len+read_this_buf->read_buf_index>single_buf_size){cout<<"ը��\n";audio_output_over=true;return;}
	if((len+read_this_buf->read_buf_index)<=read_this_buf->bufsize)
	{
		if((read_count==(bufnumber/2+1)||read_count==1)&&read_this_buf->read_buf_index<len)next_write=true;//����̶��ص㣬���͡����뻺�忪��������д���ź�

		SDL_MixAudio(stream,read_this_buf->audio_info+read_this_buf->read_buf_index,len,SDL_MIX_MAXVOLUME);//���Ĺ��ܾ䣬��֡�н�������������������stream�����
		read_this_buf->read_buf_index+= len;//ȷ����һ��Ҫ���ŵ���Ϣ
		if(read_this_buf->read_buf_index==read_this_buf->bufsize)//����պö��꣬�л��ڴ��
		{
			read_this_buf->bufsize=0;
			read_count++;if(read_count>=bufnumber)read_count=0;//����Ѿ�������ڴ���ˣ����¶�ȡ������ڴ��
			read_this_buf=&audio_buf[read_count];
			read_this_buf->read_buf_index=0;
		}
	}
	else
	{
		if(len>=single_buf_size){cout<<"����single_buf_size\n";return;}
		uint8_t temp[single_buf_size*2]={0};//������ʱ�����
		int remain_size=read_this_buf->bufsize-read_this_buf->read_buf_index;//ȷ�ϵ�ǰ��ȡ�Ļ����ʣ���С
		if(remain_size<0){cout<<"remain_size"<<remain_size<<" read_this_buf->read_buf_index"<<read_this_buf->read_buf_index<<" read_this_buf->bufsize"<<read_this_buf->bufsize<<endl;Sleep(500);return;}
		memcpy(temp,read_this_buf->audio_info+read_this_buf->read_buf_index,remain_size);//��ʣ�ಿ������copy���´�������ʱ�������
		read_this_buf->bufsize=0;
		read_this_buf->read_buf_index=0;//�û����д��ƫ������0��д�����л��ڴ����л�ǰ���ڴ��д��ƫ��������0����������0
		read_count++;if(read_count>=bufnumber)read_count=0;//����Ѿ�������ڴ���ˣ����¶�ȡ������ڴ��
		read_this_buf=&audio_buf[read_count];
		memcpy(temp+remain_size,read_this_buf->audio_info,len-remain_size);//��ȡ���ڴ���ʣ�ಿ��������len-remain_size
		SDL_MixAudio(stream,temp,len,SDL_MIX_MAXVOLUME);//����ʱ����������������Ƶ�����
		if(write_audio_buff_over&&read_this_buf->write_buf_index==0){audio_output_over=true;cout<<"������\n";}//�����Ƶû֡������һ���ڴ�����Ϊ�գ�����������׽���output_over=true
		read_this_buf->read_buf_index= len-remain_size;//ȷ����һ��Ҫ���ŵ���Ϣ
	}
}

void process_audio_function()
{
	cout<<"��ʼ�����ļ���Ƶ\n";
		//--------------------------------------------------��Ƶ���Ʋ�����ʼ��------------------------------------------------------------------
	audio_pace=0;decode_audio_pace=0;
	AudioIndex=-1;//��ʼû�ҵ���Ƶ����
	write_audio_buff_over=false;//��ǰ��Ƶ���û�н���
	next_write=false;next_read=false;//��ǰ���л��ڴ�д����Ƶ����
	audio_output_over=false;//��ǰ����ȴ��ж�û�����
	for(int i=0;i<bufnumber;i++)
	{
		memset(audio_buf[i].audio_info,0,sizeof(audio_buf[i].audio_info));//��������ڴ������
		audio_buf[i].read_buf_index=0;audio_buf[i].write_buf_index=0;//��ǰ��д�ڴ��ƫ������0
		audio_buf[i].bufsize=0;audio_buf[i].pts=0;//dts���ڴ��д���С��Ϊ0
	}
	write_this_buf=&audio_buf[0];
	read_this_buf=&audio_buf[0];
	read_count=0;write_count=0;//��Ƶ�Ķ�д�ڴ�鶼����Ϊ��һ�飬����һ�����������һ����д
	//------------------------------------------------��Ƶ���Ʋ�����ʼ������-----------------------------------------------------------------

	int ret=0;//����ffmpeg���������Ĵ�����
	char buf[1024]={0};//����ffmpeg���������Ĵ�����Ϣ
	
	AVFormatContext* pAFormatContext = avformat_alloc_context();	// ���·��������ģ�ò���޷������߳��н�����Ƶ֡�������Ĺ���

	// ���ļ�(ffmpeg�ɹ��򷵻�0),��pAFormatContext��rtspUrl
    if(avformat_open_input(&pAFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"��ʧ��,������:"<<ret;return;}
	
	cout<<"-------------------------------------ffmpeg������Ϣ-----------------------\n";
	av_dump_format(pAFormatContext, 0, rtspUrl, 0);
	cout<<"-------------------------------------ffmpeg��������-----------------------\n";

	//int audioIndex= av_find_best_stream(pAFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);if(audioIndex<0){cout<<audioIndex<<"����Ƶ\n";return;}
	for(int index = 0; index < pAFormatContext->nb_streams; index++)
    { 
        switch (pAFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_UNKNOWN\n";break;
			case AVMEDIA_TYPE_VIDEO:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_VIDEO\n";break;
			case AVMEDIA_TYPE_AUDIO:AudioIndex = index;	cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_AUDIO\n";break;
			case AVMEDIA_TYPE_DATA:						cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_DATA\n";break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_SUBTITLE\n";break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_ATTACHMENT\n";break;
			case AVMEDIA_TYPE_NB:						cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_NB\n";break;
			default:break;
        }
        if(AudioIndex >0)// �Ѿ��Ҵ���ƵƷ��,����ѭ��
		{break;}
    }//��ȡ����
	if(AudioIndex<0){cout<<"����Ƶ\n";write_audio_buff_over=true;audio_output_over=true;return;}

	AVCodecContext *pAudioCodecCtx =avcodec_alloc_context3(NULL);//�����仰�����ϰ汾������AVCodecContext *pAudioCodecCtx = pAFormatContext->streams[audioIndex]->codec;
	avcodec_parameters_to_context(pAudioCodecCtx,pAFormatContext->streams[AudioIndex]->codecpar);
	AVStream *pAStream = pAFormatContext->streams[AudioIndex];
    AVCodec *pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);//�ҵ��ʺϵĽ�����
    pAudioCodecCtx->pkt_timebase=pAFormatContext->streams[AudioIndex]->time_base;//������䣬����֣�mp3float��Could not update timestamps for skipped samples
	assert(avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr) >= 0);
	
    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;
	cout<<"��Ƶ��֡��:"	<<pAStream->nb_frames<<endl;
	cout<<"��Ƶ��ʱ��:"	<<pAStream->duration/10000.0	<<"��\n";
	cout<<"��Ƶ������:"	<<pAudioCodecCtx->sample_rate	<<endl;
	cout<<"�����ʽ:"	<<AUDIO_F32SYS					<<endl;
	cout<<"����ͨ����:"	<<pAudioCodecCtx->channels		<<endl;

	if(pAudioCodecCtx->sample_rate<=0)//mp3ò��pAudioCodecCtx->sample_rate����0�����������������������֣�
		desired_spec.freq = 44100;//���ļ���������ֵ��ȷΪ0��mp3����Ƶ������Ĭ������Ϊ44100
	else
		desired_spec.freq = pAudioCodecCtx->sample_rate;//���ļ������Լ��Ĳ�����,����ͨ���仯desired_spec.freq��ֵ���ı䲥���ٶ�,�������*x���Ƿ�x��
	audio_fre=desired_spec.freq;
    desired_spec.format = AUDIO_F32SYS;					
    desired_spec.channels = pAudioCodecCtx->channels;	
    desired_spec.silence = 0;
    desired_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    desired_spec.callback = audio_callback;//������Ƶ�ص�����Ϊaudio_callback()
	CoInitialize(NULL);//SDL�������߳��޷�ֱ��ʹ�����߳�show_moive()�г�ʼ�����SDL������������ʹ�����߳����ʼ�����SDL�������Ǿ�Ų��ᱨ��
    assert(SDL_OpenAudio(&desired_spec, &obtained_spec) >= 0);//����SDL��Ƶ����
	SDL_PauseAudio(0);
	//--------------------------------------------�ز�����ʼ����ʼ----------------------------------------------
	/*SwrContext *actx = swr_alloc();
    actx = swr_alloc_set_opts(
            actx,
            av_get_default_channel_layout(2),
            AV_SAMPLE_FMT_S16,
            pAudioCodecCtx->sample_rate,
            av_get_default_channel_layout(pAudioCodecCtx->channels),
            pAudioCodecCtx->sample_fmt,
            pAudioCodecCtx->sample_rate,
            0, 0
    );
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	av_opt_set_int(actx, "in_channel_count",pAudioCodecCtx->channels, 0);
	av_opt_set_int(actx, "out_channel_count", 1, 0);
	av_opt_set_int(actx, "in_channel_layout",pAudioCodecCtx->channel_layout, 0);
	av_opt_set_int(actx, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
	av_opt_set_int(actx, "in_sample_rate", pAudioCodecCtx->sample_rate, 0);
	av_opt_set_int(actx, "out_sample_rate", 16000, 0);//SAMPLE_RATE 
	av_opt_set_sample_fmt(actx, "in_sample_fmt", pAudioCodecCtx->sample_fmt, 0);
	av_opt_set_sample_fmt(actx, "out_sample_fmt", out_sample_fmt, 0);
	if (swr_init(actx)!= 0) {cout<<"swr_init failed";} else {cout<<"swr_init success\n";}*/
	//------------------------------------------------�ز�����ʼ������------------------------------------------
	double last_best_effort_timestamp=0;//���һ����Чʱ���
	int lost_time=0;//��ʧ��Чʱ���֡�Ĵ���

	AVPacket *pAPacket = av_packet_alloc();							// ffmpeg��֡���ݰ�
	while(av_read_frame(pAFormatContext, pAPacket) >= 0)//���ļ�����û�е�ͷʱ���������
    {
		if(pAPacket->stream_index == AudioIndex)//�����ǰ������ϢΪ��Ƶ��Ϣ�����룬������Ƶ����ӣ������ȴ�������´ν��
		{
			avcodec_send_packet(pAudioCodecCtx, pAPacket);
			AVFrame *frame = av_frame_alloc();//����֡�ռ�
			while(avcodec_receive_frame(pAudioCodecCtx, frame) >= 0)//���뿪ʼ
			{
				int uSampleSize = 4;
				int data_size = uSampleSize * pAudioCodecCtx->channels * frame->nb_samples;//ȷ�ϲ�����Ϣ�ֽ�����С
				audio_framesize=data_size;
				write_this_buf->bufsize=data_size;
				if(frame->best_effort_timestamp>0)//���ܴ���֡ʱ�����ʧ���������ֵΪ��ֵ��������ֱ����ÿx֡����һ����Чʱ������ô����֮ǰ��ʧ��
				{
					write_this_buf->pts=double(frame->best_effort_timestamp)* av_q2d(pAFormatContext->streams[AudioIndex]->time_base);//ȷ����ǰ��Чʱ���
					double gap=(double(write_this_buf->pts)-double(last_best_effort_timestamp))/double(lost_time+1);//������֮ǰ�Ǵ���Чʱ�������֡����,�������ǰһ֡ʱ����
					write_this_buf->gap=gap;//д���֡ʱ���
					last_best_effort_timestamp=write_this_buf->pts;//�������һ����Чʱ���
					decode_audio_pace=write_this_buf->pts;//ȷ�϶�����һ֡��

					int k=write_count-1;if(k<0)k=bufnumber-1;//��ʼ��ǰ��֡��ʱ�����֡�pts��gap
					double this_frame_timestamp=last_best_effort_timestamp;//time_fillΪ��֡ʱ�������ǰһ֡ʱ���Ϊtime_fill-gap
					while(audio_buf[k].gap==0&&lost_time>0)
					{
						audio_buf[k].pts=double(this_frame_timestamp)-double(gap);//����ǰһ֡ʱ���
						audio_buf[k].gap=double(gap);//����ǰһ֡ʱ����
						this_frame_timestamp-=double(gap);//��λ��֡ʱ������¸�ѭ������ǰһ֡��
						lost_time--;//�ۼƶ�ʧ��-1�������ʧ��Ϊ0���˳�ѭ��
						k--;if(k<0)k=bufnumber-1;
					}
					lost_time=0;//��ʧ����0
				}
				else//�����֡ʱ������󣬱�Ϊ���һ֡������ʱ���
				{
					lost_time++;//��ʧ��+1
					write_this_buf->pts=last_best_effort_timestamp;//�Ƚ���ǰʱ�����Ϊ���һ����Чʱ���
					write_this_buf->gap=0;//���һ֡ʱ����Ϊ0
				}

				for(int i=0;i<frame->nb_samples;i++)//����Ƶ����д���֡����
				{
					for(int j=0;j<pAudioCodecCtx->channels;j++)
					{
						if(write_this_buf->write_buf_index<write_this_buf->bufsize)//���д�뻺�����㹻ʣ��
						{
							memcpy(write_this_buf->audio_info+write_this_buf->write_buf_index, frame->data[j] + uSampleSize * i, uSampleSize);//����д��ȫ���ڴ�飬�ȴ���Ƶ�ص��������ø��ڴ���ȡ���ص�������ȡ���Զ�����
							write_this_buf->write_buf_index +=uSampleSize;//�����д��ƫ��������
							
							if(write_this_buf->write_buf_index==write_this_buf->bufsize)//�л��ڴ���
							{
								write_count++;if(write_count==bufnumber)write_count=0;
								write_this_buf=&audio_buf[write_count];
								write_this_buf->write_buf_index=0;//д��ƫ������0
								memset(write_this_buf->audio_info,0,sizeof(write_this_buf->audio_info));
								
								while(!next_write&&(write_count==bufnumber/2||write_count==bufnumber-1)){}//�л���������ˣ��ȴ��ص���������next_write�źţ����غ�д����һ�������

								if(write_count==bufnumber/2||write_count==bufnumber-1)
									{next_write=false;}//��"����Կ�ʼд��һ���ڴ����"�ź��÷�
							}
						}//else{cout<<"???"<<write_this_buf->write_buf_index<<"\t"<<write_this_buf->bufsize<<endl;Sleep(5000);}//�����ϲ���ִ�е���仰
					}
				
				}//��Ƶ֡����д�뻺�����

				
			}//����packet���

			
		}//һ֡��Ƶ�������
		
    }//���볹����ɣ������ļ����꣬��ѭ������
	if(write_count-1==-1)//�������һ֡�Ĳ��ʱ��
		write_this_buf->gap=audio_buf[bufnumber-1].gap;
	else
		write_this_buf->gap=audio_buf[write_count-1].gap;

	write_audio_buff_over=true;//��Ƶ�������
	cout<<"��Ƶд�������\n";
	while(!audio_output_over){}//�����Ƶ�ص������û����
	cout<<"SDL_Audio�ر�\n";
	SDL_CloseAudio();//��Ƶ�ر�,��ִ����仰��SDL_Quit������
}
void show_moive()//��ȡ������Ƶ
{
	video_pace=0;

    // ffmpeg��ر���Ԥ�ȶ��������
	AVFormatContext *pVFormatContext	=avformat_alloc_context();	// ffmpeg��ȫ�������ģ�����ffmpeg��������Ҫ
    AVCodecContext *pVideoCodecCtx		=0;							// ffmpeg������Ƶ������pVideoCodecCtx_live
    AVCodec *vCodec						=0;							// ffmpeg������
    //AVFrame *pAVFrame					=0;							// ffmpeg��֡����
    AVFrame *pAVFrameRGB32				=av_frame_alloc();			// ffmpeg��֡����ת����ɫ�ռ��Ļ���
    struct SwsContext *pSwsContext		=0;							// ffmpeg�������ݸ�ʽת��
    AVDictionary *pAVDictionary			=0;							// ffmpeg�����ֵ䣬��������һЩ���������Ե�
    int ret = 0;						// ����ִ�н��
	char buf[1024]={0};					// ����ִ�д���󷵻���Ϣ�Ļ���
    int videoIndex = -1;				// ��Ƶ�����ڵ����
	int numBytes = 0;					// ���������ݳ���
    unsigned char* outBuffer = 0;		// ������Ƶ������ݴ�Ż�����
	SYSTEMTIME startTime,nextTime;		// ����ǰʱ��//�������ʱ��
	int VideoTimeStamp=0;				// ʱ�������ֹ��Ƶû���Դ���pts

    if(!pVFormatContext || !pAVFrameRGB32){cout << "Failed to alloc";return;}

    // ����һ��ע�����������ͱ��������Ҳ����ֻע��һ�࣬��ע��������ע��������ȣ�
    av_register_all();
    avformat_network_init();
    // ����������ļ�(ffmpeg�ɹ��򷵻�0)
    cout << "��:" << rtspUrl<<endl;
    if(avformat_open_input(&pVFormatContext,rtspUrl, NULL, &pAVDictionary)){av_strerror(ret, buf, 1024);cout<<"��ʧ��,������:"<<ret;return;}
	// ��������̽����ý����Ϣ
    if(avformat_find_stream_info(pVFormatContext, 0)< 0){cout << "avformat_find_stream_infoʧ��\n";return;}
    
    /*ԭ�����ϴ���
	// �����ģ���ȡ����Ϣ,��ȡ��Ƶ��Ϣ
	for(int index = 0; index < pVFormatContext->nb_streams; index++)
    { 
        switch (pVFormatContext->streams[index]->codec->codec_type)
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
			pVideoCodecCtx = pVFormatContext->streams[index]->codec;
			pVStream = pVFormatContext->streams[index];break;}
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

	videoIndex=av_find_best_stream(pVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &vCodec, NULL);// �����ģ���ȡ����Ϣ,��ȡ��Ƶ��Ϣ
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
	avcodec_parameters_to_context(pVideoCodecCtx,pVFormatContext->streams[videoIndex]->codecpar);
	pVStream = pVFormatContext->streams[videoIndex];
	pVideoCodecCtx->pkt_timebase=pVFormatContext->streams[videoIndex]->time_base;

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

	AVPacket *pVPacket=av_packet_alloc();

		while(av_read_frame(pVFormatContext, pVPacket) >= 0)
		{
			//GetLocalTime(&startTime);//��ȡ����ǰ��ȷʱ��
			if(pVPacket->stream_index == videoIndex)//�����֡Ϊ��Ƶ֡����Ƶ����ʼ
			{
				ret=avcodec_send_packet(pVideoCodecCtx, pVPacket);
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
					if(double(pAVFrame->best_effort_timestamp)*av_q2d(pVStream->time_base)>0)
						video_pace=double(pAVFrame->best_effort_timestamp)*av_q2d(pVStream->time_base);
					else
						video_pace+=double(interval)/double(1000);
				}
				cout<<audio_pace<<"\t"<<video_pace<<"\t"<<decode_audio_pace<<"\t"<<decode_video_pace<<endl;
				while(video_pace>audio_pace&&!audio_output_over){}//��ֹ������ͬ��������ѭ���ȴ���ͬ��.����������������ġ�ԭʵ�ֲ����ӳٴ��롱
				if(audio_output_over){Sleep(int(interval/float(1000)));cout<<"s"<<int(interval)<<endl;}//�����Ƶ�����˵������滹û���꣬��֡���ʱ�������
		

			}//һ֡��Ƶ�������

		}//��Ƶ���볹����ɣ���ѭ������
		
	cout<<"��Ƶ�������\n";
	while(!audio_output_over){Sleep(30);}//�����Ƶû����꣬���еȴ�
	cout << "�ͷŻ�����Դ,�ر�SDL"<<endl;
	Sleep(500);
	if(pVPacket){av_free_packet(pVPacket);pVPacket = 0;}
	if(outBuffer){av_free(outBuffer);outBuffer = 0;}
	if(pSwsContext){sws_freeContext(pSwsContext);pSwsContext = 0;}
	if(pAVFrameRGB32){av_frame_free(&pAVFrameRGB32);pAVFrameRGB32 = 0;}
    if(pVideoCodecCtx){avcodec_close(pVideoCodecCtx);pVideoCodecCtx = 0;}
    if(pVFormatContext){avformat_close_input(&pVFormatContext);avformat_free_context(pVFormatContext);pVFormatContext = 0;}
    SDL_DestroyRenderer(pSDLRenderer);//������Ⱦ��
    SDL_DestroyWindow(pSDLWindow);cout<<"����ִ��SDL_Quit()\n";// ���ٴ���
	SDL_Quit(); //�˳�SDL
	
}
		/*
			if(false)//�ƶ���ָ��ʱ���
			{
				cout<<"ǿ��\n";
				int set_time=audio_pace;change=true;//��1��
				auto time_base= pVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pVFormatContext->streams[videoIndex]->start_time + av_rescale(set_time, time_base.den, time_base.num);
				ret = av_seek_frame(pVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD );
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
				auto time_base= pVFormatContext->streams[videoIndex]->time_base;
				auto seekTime = pVFormatContext->streams[videoIndex]->start_time + av_rescale(int(audio_pace), time_base.den, time_base.num);
				ret = av_seek_frame(pVFormatContext, videoIndex, seekTime, AVSEEK_FLAG_BACKWARD);
			 }*/