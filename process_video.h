#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
using namespace std;
extern "C"
{
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL.h"
#include <SDL_thread.h>
};
#pragma  comment(lib,"avcodec.lib")
#pragma  comment(lib,"avdevice.lib")
#pragma  comment(lib,"avfilter.lib")
#pragma  comment(lib,"avformat.lib")
#pragma  comment(lib,"avutil.lib")
#pragma  comment(lib,"postproc.lib")
#pragma  comment(lib,"swresample.lib")
#pragma  comment(lib,"swscale.lib")

//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\ɱ¾���У�OBD��������1280����[www.66ys.tv].rmvb";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\���Ƕ��ǳ������ߣ�.mp4";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Ӣ������ .mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\2020Ӣ����������CG��Warriors��������Ӱ�� - ��Ƶ - ����������.mp4";
char rtspUrl[] = "123123.mp4";
#define SDL_AUDIO_BUFFER_SIZE 1024
const int bufnumber=4;//�����ڴ��Ϊ4��
static int audio_buf_read_index[bufnumber],audio_buf_write_index[bufnumber];//�ڴ���ڲ���дƫ����
static uint8_t audiobuf[bufnumber][44100*16*2*4];//�ڴ��
static int write_count,read_count;//��ǰ����д���ڴ��
static bool next_write;//��Ƶд���ڴ������Լ�����
static bool output_over;//��Ƶ���������
static int get_number,queque_number;//get_number��ȡ���кš�queque_number��ǰ���к�
static int video_pace,audio_pace;//ȷ�ϵ�ǰ���ȣ���֤��Ƶ��Ƶͬ��
static int AudioIndex,VideoIndex;//ȫ��ȷ�ϸ�֡�Ƿ�����Ƶ֡����Ƶ֡
bool audio_over;//��Ƶ�Ƿ������trueΪ������falseΪû����
static int output_number,audio_fre;//�ص���Ƶlen�ۼƲ�����,��Ƶ������

const int my_width=1024;//�Լ����Էֱ��ʣ�����������ʾʱ��������
const int my_height=768;



void audio_callback(void *userdata, Uint8 *stream, int len) 
{
	int my_number=get_number;//�ӷ����ȡ����
	if(get_number==10000)//�������������
		get_number=0;//�����������Ϊ0
	else
		get_number++;//�������+1
	while(queque_number!=my_number){};//���к���û�ֵ��Լ������еȴ�
	if(output_over)return;
	if(audio_fre<(len+output_number))
	{
		audio_pace++;
		output_number=len+output_number-audio_fre;
	}
	else
		output_number=output_number+len;
	

	//memcpy(stream, audiobuf[read_count]+audio_buf_read_index, len);//���Լ�Я����Ϣ���в���,��˵memcpy����ֱ���������ʧ�棬�������������
	SDL_memset(stream, 0, len);
	SDL_MixAudio(stream,audiobuf[read_count]+audio_buf_read_index[read_count],len,SDL_MIX_MAXVOLUME);//���Ĺ��ܾ䣬��֡�н�������������������stream�����

	audio_buf_read_index[read_count]+= len;//ȷ����һ��Ҫ���ŵ���Ϣ
	int let_write=5;//��ǰ����ʱ�䣬��д����Ƶ�������д�룬��ֹ��Ƶ�������
	if(audio_buf_read_index[read_count]>(audio_buf_write_index[read_count]-len*let_write))//�����ȡ�ڴ�鼴����ͷ�����len*5�������п��ƣ�5�ĳ�6��7֮��ģ���֤�л�ʱ������
		if(audio_buf_read_index[read_count]>=(audio_buf_write_index[read_count]-len))//����ڴ泹�׵�ͷ��û��һ��Ҫ���ŵ���Ϣ�ˣ��л��ڴ���
		{
			audio_buf_read_index[read_count]=0;//�ڴ�ƫ������0
			audio_buf_write_index[read_count]=0;//�ڴ�ƫ������0
			memset(audiobuf[read_count],0,sizeof(audiobuf[read_count]));
			read_count++;//�л���һ���ڴ����ж�ȡ
			if(read_count>=bufnumber)read_count=0;//����Ѿ�������ڴ���ˣ����¶�ȡ������ڴ��
			audio_buf_read_index[read_count]=0;
			cout<<read_count<<"�л���\n";
			if(audio_over&&audio_buf_write_index[read_count]==0){output_over=true;cout<<"������\n";}
		}
		else
			if(audio_buf_read_index[read_count]<=(audio_buf_write_index[read_count]-len*(let_write-1)))
				next_write=true;//����ڴ�鼴����ͷ������û���׵�ͷ����ĳһ�ض�λ�ã�֪ͨ��Ƶд���ڴ������Լ�����
	if(queque_number==10000)
		{queque_number=0;}//���к���ˢ��Ϊ-1
	else
		queque_number++;//���к���+1����һ���õ�����Ŀ��Կ�ʼ������
}

void process_audio_function()
{
	cout<<"��ʼ�����ļ���Ƶ\n";

	int ret=0;//����ffmpeg���������Ĵ�����
	char buf[1024]={0};//����ffmpeg���������Ĵ�����Ϣ

	AVPacket *pAPacket = av_packet_alloc();;                        // ffmpag��֡���ݰ�
	AVFormatContext* pAFormatContext = avformat_alloc_context();    // ���·��������ģ�ò���޷������߳��н�����Ƶ֡�������Ĺ���
	//----------------------------------------------------------------------------------------------------------
	// ���ļ�(ffmpeg�ɹ��򷵻�0),��pAFormatContext��rtspUrl
    if(avformat_open_input(&pAFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"��ʧ��,������:"<<ret;return;}
	
	cout<<"-------------------------------------ffmpeg������Ϣ-----------------------\n";
	av_dump_format(pAFormatContext, 0, rtspUrl, 0);
	cout<<"-------------------------------------ffmpeg��������-----------------------\n";

	int audioIndex ;//= av_find_best_stream(pAFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);if(audioIndex<0){cout<<audioIndex<<"����Ƶ\n";return;}
	for(int index = 0; index < pAFormatContext->nb_streams; index++)
    { 
        switch (pAFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_UNKNOWN\n";break;
			case AVMEDIA_TYPE_VIDEO:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_VIDEO\n";break;
			case AVMEDIA_TYPE_AUDIO:audioIndex = index;	cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_AUDIO\n";break;
			case AVMEDIA_TYPE_DATA:						cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_DATA\n";break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_SUBTITLE\n";break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_ATTACHMENT\n";break;
			case AVMEDIA_TYPE_NB:						cout << "�����:" << index << "����Ϊ:AVMEDIA_TYPE_NB\n";break;
			default:break;
        }
        if(audioIndex >0)// �Ѿ��Ҵ���ƵƷ��,����ѭ��
		{break;}
    }//��ȡ����
	AudioIndex=audioIndex;
	if(audioIndex<0){cout<<"����Ƶ\n";audio_over=true;output_over=true;return;}

	AVCodecContext *pAudioCodecCtx =avcodec_alloc_context3(NULL);//�����仰�����ϰ汾������AVCodecContext *pAudioCodecCtx = pAFormatContext->streams[audioIndex]->codec;
	avcodec_parameters_to_context(pAudioCodecCtx,pAFormatContext->streams[audioIndex]->codecpar);
	
    AVCodec *pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);//�ҵ��ʺϵĽ�����
    assert(avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr) >= 0);

    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;

	cout<<"��Ƶ������:"	<<pAudioCodecCtx->sample_rate	<<endl;
	cout<<"�����ʽ:"	<<AUDIO_F32SYS					<<endl;
	cout<<"����ͨ����:"	<<pAudioCodecCtx->channels		<<endl;

	if(pAudioCodecCtx->sample_rate<=0)//mp3ò��pAudioCodecCtx->sample_rate����0�����������������������֣�
		desired_spec.freq = 44100;//���ļ���������ֵ��ȷΪ0��mp3����Ƶ������Ĭ������Ϊ44100
	else
		desired_spec.freq = pAudioCodecCtx->sample_rate;//���ļ������Լ��Ĳ�����,����ͨ���仯desired_spec.freq��ֵ���ı䲥���ٶ�,�������*x���Ƿ�x��
    desired_spec.format = AUDIO_F32SYS;					
    desired_spec.channels = pAudioCodecCtx->channels;	
    desired_spec.silence = 0;
    desired_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    desired_spec.callback = audio_callback;//������Ƶ�ص�����Ϊaudio_callback()
	CoInitialize(NULL);//SDL�������߳��޷�ֱ��ʹ�����߳�show_moive()�г�ʼ�����SDL������������ʹ�����߳����ʼ�����SDL�������Ǿ�Ų��ᱨ��
    assert(SDL_OpenAudio(&desired_spec, &obtained_spec) >= 0);

	SDL_PauseAudio(0);
	//----------------------------------------------------------------------------------------------------------
	while(av_read_frame(pAFormatContext, pAPacket) >= 0)//���ļ�����û�е�ͷʱ���������
    {

		if(pAPacket->stream_index == audioIndex)//�����ǰ������ϢΪ��Ƶ��Ϣ�����룬������Ƶ����ӣ������ȴ�������´ν��
		{
				
			avcodec_send_packet(pAudioCodecCtx, pAPacket);
			AVFrame *frame = av_frame_alloc();//����֡�ռ�
			while(avcodec_receive_frame(pAudioCodecCtx, frame) >= 0)//���뿪ʼ
			{
				int uSampleSize = 4;
				int data_size = uSampleSize * pAudioCodecCtx->channels * frame->nb_samples;//ȷ�ϲ�����Ϣ�ֽ�����С
				audio_fre=data_size;
				uint8_t *pBufferIterator= audiobuf[write_count]+audio_buf_write_index[write_count];//����ָ�룬ָ��ǰ�ڴ���ûд�Ĳ���
				for(int i=0;i<frame->nb_samples;i++)
				{
					for(int j=0;j<pAudioCodecCtx->channels;j++)
					{
						memcpy(pBufferIterator, frame->data[j] + uSampleSize * i, uSampleSize);//����Ƶ��Ϣѹ���ڴ������ȴ��ص�������ȡ�����ݲ����в���
						pBufferIterator += uSampleSize;//ָ��ƫ��������
					}
				}
				audio_buf_write_index[write_count] += data_size;//�ڴ�ƫ��������
				
				if(audio_buf_write_index[write_count]>(sizeof(audiobuf[0])-data_size))//���ƫ�ƺ��ڴ�������ˣ��л��ڴ�飬һ��bufnumber���ڴ���
				{
					write_count++;//�л���ǰ�ڴ��
					if(write_count>=bufnumber)write_count=0;//����ڴ��ﵽbufnumber����,�������׿�����
					audio_buf_write_index[write_count]=0;//�ڴ���ڲ�ƫ������0
					memset(audiobuf[write_count],0,sizeof(audiobuf[write_count]));//��ռ���Ҫд����ڴ���ԭ������
					cout<<write_count<<"�л�д\n";//��ӡ�л���¼
					while(!next_write){}
					next_write=false;
				}//�л��ڴ�����
			}//�������
		}//һ֡��Ƶ�������
			
			
    }//���볹����ɣ������ļ����꣬��ѭ������
	audio_over=true;//��Ƶ�������
	while(!output_over){}//�����Ƶ�ص������û����
	cout<<"SDL_Audio�ر�\n";
	SDL_CloseAudio();//��Ƶ�ر�,��ִ����仰��SDL_Quit������
}

void show_moive()//��ȡ������Ƶ
{
	video_pace=0;audio_pace=0;
	//--------------------------------------------------��Ƶ���Ʋ�����ʼ��------------------------------------------------------------------
	audio_over=false;//��ǰ��Ƶ���û�н���
	next_write=false;//��ǰ���л��ڴ�д����Ƶ����
	output_over=false;//��ǰ����ȴ��ж�û�����
	output_number=0;//�ص��ۼƲ��������
	for(int i=0;i<bufnumber;i++)memset(audiobuf[i],0,sizeof(audiobuf[i]));
	read_count=0;write_count=0;//��Ƶ�Ķ�д�ڴ�鶼����Ϊ��һ�飬����һ�����������һ����д
	get_number=0;queque_number=0;//��Ƶ�ص��������з���
	audio_buf_read_index[0]=0;audio_buf_write_index[0]=0;//��Ƶ�ڴ��ƫ������ʼ��Ϊ0
	//------------------------------------------------��Ƶ���Ʋ�����ʼ������-----------------------------------------------------------------

    // ffmpeg��ر���Ԥ�ȶ��������
	AVFormatContext *pAVFormatContext	=avformat_alloc_context();	// ffmpeg��ȫ�������ģ�����ffmpeg��������Ҫ
	AVPacket *pAVPacket					=av_packet_alloc();			// ffmpag��֡���ݰ�
    AVStream *pAVStream					=0;							// ffmpeg����Ϣ����ȷ����Ҫ������Ϣ���ٽ��и�ֵ
	AVFrame *pAVFrame					=av_frame_alloc();			// ffmpeg��֡��ʽ
    AVCodecContext *vCodecCtx			=0;							// ffmpeg����������
    AVCodec *vCodec						=0;							// ffmpeg������
    //AVFrame *pAVFrame					=0;							// ffmpeg��֡����
    AVFrame *pAVFrameRGB32				=av_frame_alloc();			// ffmpeg��֡����ת����ɫ�ռ��Ļ���
    struct SwsContext *pSwsContext		=0;							// ffmpeg�������ݸ�ʽת��
    AVDictionary *pAVDictionary			=0;							// ffmpeg�����ֵ䣬��������һЩ���������Ե�
    int ret = 0;						// ����ִ�н��
	char buf[1024]={0};					// ����ִ�д���󷵻���Ϣ�Ļ���
    int videoIndex = -1;				// ��Ƶ�����ڵ����
    int audioIndex = -1;				// ��Ƶ�����ڵ����
	int numBytes = 0;					// ���������ݳ���
    unsigned char* outBuffer = 0;		// ���������ݴ�Ż�����
	SYSTEMTIME startTime,nextTime;		// ����ǰʱ��//�������ʱ��
	
	//__int64 startTime = 0;			// ��¼���ſ�ʼ

    if(!pAVFormatContext || !pAVPacket || !pAVFrame || !pAVFrameRGB32){cout << "Failed to alloc";return;}

    // ����һ��ע�����������ͱ��������Ҳ����ֻע��һ�࣬��ע��������ע��������ȣ�
    av_register_all();
    avformat_network_init();
    // ����������ļ�(ffmpeg�ɹ��򷵻�0)
    cout << "��:" << rtspUrl<<endl;
    if(avformat_open_input(&pAVFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"��ʧ��,������:"<<ret;return;}
    // ��������̽����ý����Ϣ
    if(avformat_find_stream_info(pAVFormatContext, 0)< 0){cout << "avformat_find_stream_infoʧ��\n";return;}
    // �����ģ���ȡ����Ϣ,��ȡ��Ƶ��Ϣ
    for(int index = 0; index < pAVFormatContext->nb_streams; index++)
    { 
        switch (pAVFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_UNKNOWN"<<endl;break;
			case AVMEDIA_TYPE_VIDEO:videoIndex = index;	cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_VIDEO"<<endl;break;
			case AVMEDIA_TYPE_AUDIO:					cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_AUDIO"<<endl;break;
			case AVMEDIA_TYPE_DATA:						cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_DATA"<<endl;break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_SUBTITLE"<<endl;break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_ATTACHMENT"<<endl;break;
			case AVMEDIA_TYPE_NB:						cout << "\n�����:" << index << "����Ϊ:" << "AVMEDIA_TYPE_NB"<<endl;break;
			default:break;
        }
        if(videoIndex != -1)// �Ѿ��Ҵ���ƵƷ��,����ѭ��
		{vCodecCtx = pAVFormatContext->streams[index]->codec;pAVStream = pAVFormatContext->streams[index];break;}
    }//��ȡ����
	VideoIndex=videoIndex;
	if(videoIndex == -1 || !vCodecCtx){cout << "Failed to find video stream";return;}//�ж��Ƿ��ҵ���Ƶ��
	
    // �����壺���ҵ�����Ƶ��Ѱ������
    vCodec = avcodec_find_decoder(vCodecCtx->codec_id);
    if(!vCodec){cout<<"Failed to avcodec_find_decoder(vCodecCtx->codec_id):"<< vCodecCtx->codec_id;return; }

    // ���������򿪽�����    
    av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);// ���û����С 1024000byte    
    av_dict_set(&pAVDictionary, "stimeout", "20000000", 0);// ���ó�ʱʱ�� 20s    
    av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);// ���������ʱ 3s    
    av_dict_set(&pAVDictionary, "rtsp_transport", "tcp", 0);// ���ô򿪷�ʽ tcp/udp
    if(avcodec_open2(vCodecCtx, vCodec, &pAVDictionary)){cout << "Failed to avcodec_open2(vCodecCtx, vCodec, pAVDictionary)";return;}
    
    // ��ʾ��Ƶ��صĲ�����Ϣ�����������ģ�                            
    cout<<"������:"		<<vCodecCtx->bit_rate			<<endl;
    cout<<"��ʱ��:"		<<pAVStream->duration/10000.0	<<"��\n";
    cout<<"��֡��:"		<<pAVStream->nb_frames			<<endl;	
    cout<<"��ʽ:"		<<vCodecCtx->pix_fmt			<<endl;//��ʽΪ0˵����AV_PIX_FMT_YUV420P
    cout<<"��:"			<<vCodecCtx->width				<<"\t�ߣ�"		<<vCodecCtx->height				<<endl;
	cout<<"�ļ���ĸ:"	<<vCodecCtx->time_base.den		<<"\t�ļ�����:"	<<vCodecCtx->time_base.num		<<endl;
    cout<<"֡�ʷ�ĸ:"	<<pAVStream->avg_frame_rate.den	<<"\t֡�ʷ���:"	<<pAVStream->avg_frame_rate.num	<<endl;
    // ����ʱ����ʱ�����֡�ʣ���Ϊ׼ȷ��
	double fps=pAVStream->avg_frame_rate.num * 1.0f / pAVStream->avg_frame_rate.den;if(fps<=0){cout<<fps<<endl;return;}//֡��
    double interval = 1 * 1000 / fps;//֡���
    cout<<"ƽ��֡��:"	<< fps<<endl;
    cout<<"֡���:"		<< interval << "ms"<<endl;

    // �����ߣ����õ���ԭʼ���ݸ�ʽ��������ת��Ϊָ���ĸ�ʽ�߿��С  AV_PIX_FMT_YUV420P        vCodecCtx->pix_fmt AV_PIX_FMT_RGBA
    pSwsContext = sws_getContext(vCodecCtx->width,vCodecCtx->height,vCodecCtx->pix_fmt,vCodecCtx->width,vCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_FAST_BILINEAR,0,0,0);
    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,vCodecCtx->width,vCodecCtx->height);
    outBuffer = (unsigned char *)av_malloc(numBytes);
    avpicture_fill((AVPicture *)pAVFrameRGB32,outBuffer,AV_PIX_FMT_YUV420P,vCodecCtx->width,vCodecCtx->height);//pAVFrame32��dataָ��ָ����outBuffer

	//---------------------------------------------------------SDL��ر���Ԥ�ȶ���----------------------------------------------------------------
    SDL_Window *pSDLWindow = 0;
    SDL_Renderer *pSDLRenderer = 0;
    SDL_Surface *pSDLSurface = 0;
    SDL_Texture *pSDLTexture = 0;
    SDL_Event event;

    if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//��ʼ��SDL 
		{cout<<"Failed SDL_Init\n";				return;}
	int set_width,set_height;
	if(vCodecCtx->width>my_width)set_width=my_width;else set_width=vCodecCtx->width;
	if(vCodecCtx->height>my_height) set_height=my_height;else set_height=vCodecCtx->height;
	pSDLWindow = SDL_CreateWindow("ZasLeonPlayer",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,set_width,set_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);//������ʾ���С
    if(!pSDLWindow)
		{cout<<"Failed SDL_CreateWindow\n";		return;}
    pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
	if(!pSDLRenderer)
		{cout<<"Failed SDL_CreateRenderer\n";	return;}
    pSDLTexture = SDL_CreateTexture(pSDLRenderer,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,vCodecCtx->width,vCodecCtx->height);
	if(!pSDLTexture)
		{cout<<"Failed SDL_CreateTexture\n";	return;}
	//---------------------------------------------------------SDL�������------------------------------------------------------------------------

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//��ʼ��Ƶ�̲߳��𲢲���
   
	// ����ˣ���ȡһ֡���ݵ����ݰ�
    while(av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
		if(pAVPacket->stream_index ==AudioIndex)video_pace++;
		
		GetLocalTime(&startTime);//��ȡ����ǰ��ȷʱ��
        if(pAVPacket->stream_index == videoIndex)//�����֡Ϊ��Ƶ֡����Ƶ����ʼ
        {
			ret=avcodec_send_packet(vCodecCtx, pAVPacket);
            if(ret){cout << "Failed to avcodec_send_packet(vCodecCtx, pAVPacket) ,ret =" << ret;break;}
			
            while(!avcodec_receive_frame(vCodecCtx, pAVFrame))
            {
                sws_scale(pSwsContext,(const uint8_t * const *)pAVFrame->data,pAVFrame->linesize,0,vCodecCtx->height,pAVFrameRGB32->data,pAVFrameRGB32->linesize);

                SDL_UpdateYUVTexture(pSDLTexture,NULL,pAVFrame->data[0], pAVFrame->linesize[0],pAVFrame->data[1], pAVFrame->linesize[1],pAVFrame->data[2], pAVFrame->linesize[2]);

                SDL_RenderClear(pSDLRenderer);
                // Texture���Ƶ�Renderer
                SDL_Rect sdlRect;
                sdlRect.x=0;
				sdlRect.y=0;
				if(pAVFrame->width>my_width)sdlRect.w=my_width;else sdlRect.w=pAVFrame->width;
				if(pAVFrame->height>my_height)sdlRect.h = my_height;else sdlRect.h = pAVFrame->height;
				
                SDL_RenderCopy(pSDLRenderer, pSDLTexture, 0, &sdlRect) ;
                SDL_RenderPresent(pSDLRenderer);// ����Renderer��ʾ
                SDL_PollEvent(&event);// �¼�����
            }   
			if(output_over)//�����Ƶ���������ͼƬ��ͼƬ֡ʱ����еȴ���ˢ�²���
			{
				GetLocalTime(&nextTime);//��ȡ�����ȷʱ��
				if(nextTime.wMilliseconds<startTime.wMilliseconds)nextTime.wMilliseconds=nextTime.wMilliseconds+1000;//�����������С�ڽ���ǰ�������ʱ��+1000���벹
				int delaytime=int(interval)-int(nextTime.wMilliseconds-startTime.wMilliseconds);//�������ǰ������
				if(delaytime<interval&&delaytime>0)Sleep(delaytime);//�����ӳ٣���ֹ�����ٶȹ��죬�������delaytime-1�û����һ��ˢ�£����������ѭ������ͬ��
			}
			while(video_pace>audio_pace){if(video_pace==audio_pace)break;}//��ֹ������ͬ��������ѭ���ȴ���ͬ�����������������������΢�Ȼ����һ֡audio_pace+1
        }//һ֡��Ƶ�������

    }//��Ƶ���볹����ɣ���ѭ������
	cout<<"��Ƶ�������\n";
	while(!output_over){SDL_PollEvent(&event);Sleep(30);}//�����Ƶû����꣬���еȴ�
	cout << "�ͷŻ�����Դ,�ر�SDL"<<endl;
	Sleep(500);
	if(outBuffer){av_free(outBuffer);outBuffer = 0;}
	if(pSwsContext){sws_freeContext(pSwsContext);pSwsContext = 0;}
	if(pAVFrameRGB32){av_frame_free(&pAVFrameRGB32);pAVFrame = 0;}
	if(pAVFrame){av_frame_free(&pAVFrame);pAVFrame = 0;} 
    if(pAVPacket){av_free_packet(pAVPacket);pAVPacket = 0;}
    if(vCodecCtx){avcodec_close(vCodecCtx);vCodecCtx = 0;}
    if(pAVFormatContext){avformat_close_input(&pAVFormatContext);avformat_free_context(pAVFormatContext);pAVFormatContext = 0;}
	
    // �����壺������Ⱦ��
    SDL_DestroyRenderer(pSDLRenderer);
    // �����������ٴ���
    SDL_DestroyWindow(pSDLWindow);cout<<"����ִ��SDL_Quit()\n";
    // �����ߣ��˳�SDL
    SDL_Quit();
	
}