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

//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\杀戮都市：OBD日语中字1280高清[www.66ys.tv].rmvb";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\我们都是超能力者！.mp4";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\英雄联盟 .mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\2020英雄联盟赛季CG《Warriors》动画电影。 - 视频 - 网易云音乐.mp4";
char rtspUrl[] = "123123.mp4";
#define SDL_AUDIO_BUFFER_SIZE 1024
const int bufnumber=4;//设置内存块为4个
static int audio_buf_read_index[bufnumber],audio_buf_write_index[bufnumber];//内存块内部读写偏移量
static uint8_t audiobuf[bufnumber][44100*16*2*4];//内存块
static int write_count,read_count;//当前读或写的内存块
static bool next_write;//音频写入内存服务可以继续了
static bool output_over;//音频输出结束了
static int get_number,queque_number;//get_number获取队列号、queque_number当前队列号
static int video_pace,audio_pace;//确认当前进度，保证视频音频同步
static int AudioIndex,VideoIndex;//全局确认该帧是否是音频帧或视频帧
bool audio_over;//音频是否结束，true为结束，false为没结束
static int output_number,audio_fre;//回调音频len累计播放数,音频采样率

const int my_width=1024;//自己电脑分辨率，超过则在显示时进行限制
const int my_height=768;



void audio_callback(void *userdata, Uint8 *stream, int len) 
{
	int my_number=get_number;//从服务获取号码
	if(get_number==10000)//如果服务号码过大
		get_number=0;//服务号码重置为0
	else
		get_number++;//服务号码+1
	while(queque_number!=my_number){};//队列号码没轮到自己，进行等待
	if(output_over)return;
	if(audio_fre<(len+output_number))
	{
		audio_pace++;
		output_number=len+output_number-audio_fre;
	}
	else
		output_number=output_number+len;
	

	//memcpy(stream, audiobuf[read_count]+audio_buf_read_index, len);//将自己携带信息进行播放,据说memcpy这样直接做会造成失真，用以下两句代替
	SDL_memset(stream, 0, len);
	SDL_MixAudio(stream,audiobuf[read_count]+audio_buf_read_index[read_count],len,SDL_MIX_MAXVOLUME);//核心功能句，将帧中解析出来的数据流输入stream并输出

	audio_buf_read_index[read_count]+= len;//确定下一段要播放的信息
	int let_write=5;//提前多少时间，让写入音频服务进行写入，防止音频输出卡顿
	if(audio_buf_read_index[read_count]>(audio_buf_write_index[read_count]-len*let_write))//如果读取内存块即将到头，这个len*5可以自行控制，5改成6或7之类的，保证切换时不卡顿
		if(audio_buf_read_index[read_count]>=(audio_buf_write_index[read_count]-len))//如果内存彻底到头，没下一段要播放的信息了，切换内存条
		{
			audio_buf_read_index[read_count]=0;//内存偏移量置0
			audio_buf_write_index[read_count]=0;//内存偏移量置0
			memset(audiobuf[read_count],0,sizeof(audiobuf[read_count]));
			read_count++;//切换下一个内存块进行读取
			if(read_count>=bufnumber)read_count=0;//如果已经是最大内存块了，重新读取最初的内存块
			audio_buf_read_index[read_count]=0;
			cout<<read_count<<"切换读\n";
			if(audio_over&&audio_buf_write_index[read_count]==0){output_over=true;cout<<"结束了\n";}
		}
		else
			if(audio_buf_read_index[read_count]<=(audio_buf_write_index[read_count]-len*(let_write-1)))
				next_write=true;//如果内存块即将到头，但还没彻底到头，在某一特定位置，通知音频写入内存服务可以继续了
	if(queque_number==10000)
		{queque_number=0;}//队列号码刷新为-1
	else
		queque_number++;//队列号码+1，下一个拿到号码的可以开始播放了
}

void process_audio_function()
{
	cout<<"开始分析文件音频\n";

	int ret=0;//储存ffmpeg操作发来的错误码
	char buf[1024]={0};//储存ffmpeg操作发来的错误信息

	AVPacket *pAPacket = av_packet_alloc();;                        // ffmpag单帧数据包
	AVFormatContext* pAFormatContext = avformat_alloc_context();    // 重新分配上下文，貌似无法与主线程中解析视频帧的上下文共用
	//----------------------------------------------------------------------------------------------------------
	// 打开文件(ffmpeg成功则返回0),绑定pAFormatContext和rtspUrl
    if(avformat_open_input(&pAFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"打开失败,错误编号:"<<ret;return;}
	
	cout<<"-------------------------------------ffmpeg分析信息-----------------------\n";
	av_dump_format(pAFormatContext, 0, rtspUrl, 0);
	cout<<"-------------------------------------ffmpeg分析结束-----------------------\n";

	int audioIndex ;//= av_find_best_stream(pAFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);if(audioIndex<0){cout<<audioIndex<<"无音频\n";return;}
	for(int index = 0; index < pAFormatContext->nb_streams; index++)
    { 
        switch (pAFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_UNKNOWN\n";break;
			case AVMEDIA_TYPE_VIDEO:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_VIDEO\n";break;
			case AVMEDIA_TYPE_AUDIO:audioIndex = index;	cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_AUDIO\n";break;
			case AVMEDIA_TYPE_DATA:						cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_DATA\n";break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_SUBTITLE\n";break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_ATTACHMENT\n";break;
			case AVMEDIA_TYPE_NB:						cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_NB\n";break;
			default:break;
        }
        if(audioIndex >0)// 已经找打视频品流,跳出循环
		{break;}
    }//提取结束
	AudioIndex=audioIndex;
	if(audioIndex<0){cout<<"无音频\n";audio_over=true;output_over=true;return;}

	AVCodecContext *pAudioCodecCtx =avcodec_alloc_context3(NULL);//此两句话可用老版本概括：AVCodecContext *pAudioCodecCtx = pAFormatContext->streams[audioIndex]->codec;
	avcodec_parameters_to_context(pAudioCodecCtx,pAFormatContext->streams[audioIndex]->codecpar);
	
    AVCodec *pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);//找到适合的解码器
    assert(avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr) >= 0);

    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;

	cout<<"音频采样率:"	<<pAudioCodecCtx->sample_rate	<<endl;
	cout<<"输出格式:"	<<AUDIO_F32SYS					<<endl;
	cout<<"声音通道数:"	<<pAudioCodecCtx->channels		<<endl;

	if(pAudioCodecCtx->sample_rate<=0)//mp3貌似pAudioCodecCtx->sample_rate都是0？导致输出的声音很慢很奇怪！
		desired_spec.freq = 44100;//该文件采样率数值的确为0，mp3的音频采样率默认设置为44100
	else
		desired_spec.freq = pAudioCodecCtx->sample_rate;//该文件有它自己的采样率,可以通过变化desired_spec.freq的值来改变播放速度,后面加上*x就是翻x倍
    desired_spec.format = AUDIO_F32SYS;					
    desired_spec.channels = pAudioCodecCtx->channels;	
    desired_spec.silence = 0;
    desired_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    desired_spec.callback = audio_callback;//设置音频回调函数为audio_callback()
	CoInitialize(NULL);//SDL操作，线程无法直接使用主线程show_moive()中初始化后的SDL，加了这句才能使用主线程里初始化后的SDL，下面那句才不会报错
    assert(SDL_OpenAudio(&desired_spec, &obtained_spec) >= 0);

	SDL_PauseAudio(0);
	//----------------------------------------------------------------------------------------------------------
	while(av_read_frame(pAFormatContext, pAPacket) >= 0)//当文件内容没有到头时，解包分析
    {

		if(pAPacket->stream_index == audioIndex)//如果当前包内信息为音频信息，解码，不是音频则忽视，经过等待后进行下次解包
		{
				
			avcodec_send_packet(pAudioCodecCtx, pAPacket);
			AVFrame *frame = av_frame_alloc();//分配帧空间
			while(avcodec_receive_frame(pAudioCodecCtx, frame) >= 0)//解码开始
			{
				int uSampleSize = 4;
				int data_size = uSampleSize * pAudioCodecCtx->channels * frame->nb_samples;//确认播放信息字节数大小
				audio_fre=data_size;
				uint8_t *pBufferIterator= audiobuf[write_count]+audio_buf_write_index[write_count];//创建指针，指向当前内存条没写的部分
				for(int i=0;i<frame->nb_samples;i++)
				{
					for(int j=0;j<pAudioCodecCtx->channels;j++)
					{
						memcpy(pBufferIterator, frame->data[j] + uSampleSize * i, uSampleSize);//将音频信息压入内存条，等待回调函数读取该数据并进行播放
						pBufferIterator += uSampleSize;//指针偏移量增加
					}
				}
				audio_buf_write_index[write_count] += data_size;//内存偏移量增加
				
				if(audio_buf_write_index[write_count]>(sizeof(audiobuf[0])-data_size))//如果偏移后内存块用完了，切换内存块，一共bufnumber块内存条
				{
					write_count++;//切换当前内存块
					if(write_count>=bufnumber)write_count=0;//如果内存块达到bufnumber上限,重新往首块输入
					audio_buf_write_index[write_count]=0;//内存块内部偏移量置0
					memset(audiobuf[write_count],0,sizeof(audiobuf[write_count]));//清空即将要写入的内存块的原有内容
					cout<<write_count<<"切换写\n";//打印切换记录
					while(!next_write){}
					next_write=false;
				}//切换内存块结束
			}//解码完成
		}//一帧音频处理结束
			
			
    }//解码彻底完成，整个文件读完，死循环结束
	audio_over=true;//音频解码结束
	while(!output_over){}//如果音频回调输出还没结束
	cout<<"SDL_Audio关闭\n";
	SDL_CloseAudio();//音频关闭,不执行这句话，SDL_Quit将阻塞
}

void show_moive()//读取本地视频
{
	video_pace=0;audio_pace=0;
	//--------------------------------------------------音频控制参数初始化------------------------------------------------------------------
	audio_over=false;//当前音频输出没有结束
	next_write=false;//当前不切换内存写入音频数据
	output_over=false;//当前输出等待列队没输出完
	output_number=0;//回调累计播放数清空
	for(int i=0;i<bufnumber;i++)memset(audiobuf[i],0,sizeof(audiobuf[i]));
	read_count=0;write_count=0;//音频的读写内存块都设置为第一块，往第一块里读，往第一块里写
	get_number=0;queque_number=0;//音频回调函数队列分配
	audio_buf_read_index[0]=0;audio_buf_write_index[0]=0;//音频内存块偏移量初始化为0
	//------------------------------------------------音频控制参数初始化结束-----------------------------------------------------------------

    // ffmpeg相关变量预先定义与分配
	AVFormatContext *pAVFormatContext	=avformat_alloc_context();	// ffmpeg的全局上下文，所有ffmpeg操作都需要
	AVPacket *pAVPacket					=av_packet_alloc();			// ffmpag单帧数据包
    AVStream *pAVStream					=0;							// ffmpeg流信息，在确认想要的流信息后再进行赋值
	AVFrame *pAVFrame					=av_frame_alloc();			// ffmpeg单帧格式
    AVCodecContext *vCodecCtx			=0;							// ffmpeg编码上下文
    AVCodec *vCodec						=0;							// ffmpeg编码器
    //AVFrame *pAVFrame					=0;							// ffmpeg单帧缓存
    AVFrame *pAVFrameRGB32				=av_frame_alloc();			// ffmpeg单帧缓存转换颜色空间后的缓存
    struct SwsContext *pSwsContext		=0;							// ffmpeg编码数据格式转换
    AVDictionary *pAVDictionary			=0;							// ffmpeg数据字典，用于配置一些编码器属性等
    int ret = 0;						// 函数执行结果
	char buf[1024]={0};					// 函数执行错误后返回信息的缓存
    int videoIndex = -1;				// 视频流所在的序号
    int audioIndex = -1;				// 音频流所在的序号
	int numBytes = 0;					// 解码后的数据长度
    unsigned char* outBuffer = 0;		// 解码后的数据存放缓存区
	SYSTEMTIME startTime,nextTime;		// 解码前时间//解码完成时间
	
	//__int64 startTime = 0;			// 记录播放开始

    if(!pAVFormatContext || !pAVPacket || !pAVFrame || !pAVFrameRGB32){cout << "Failed to alloc";return;}

    // 步骤一：注册所有容器和编解码器（也可以只注册一类，如注册容器、注册编码器等）
    av_register_all();
    avformat_network_init();
    // 步骤二：打开文件(ffmpeg成功则返回0)
    cout << "打开:" << rtspUrl<<endl;
    if(avformat_open_input(&pAVFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"打开失败,错误编号:"<<ret;return;}
    // 步骤三：探测流媒体信息
    if(avformat_find_stream_info(pAVFormatContext, 0)< 0){cout << "avformat_find_stream_info失败\n";return;}
    // 步骤四：提取流信息,提取视频信息
    for(int index = 0; index < pAVFormatContext->nb_streams; index++)
    { 
        switch (pAVFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_UNKNOWN"<<endl;break;
			case AVMEDIA_TYPE_VIDEO:videoIndex = index;	cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_VIDEO"<<endl;break;
			case AVMEDIA_TYPE_AUDIO:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_AUDIO"<<endl;break;
			case AVMEDIA_TYPE_DATA:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_DATA"<<endl;break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_SUBTITLE"<<endl;break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_ATTACHMENT"<<endl;break;
			case AVMEDIA_TYPE_NB:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_NB"<<endl;break;
			default:break;
        }
        if(videoIndex != -1)// 已经找打视频品流,跳出循环
		{vCodecCtx = pAVFormatContext->streams[index]->codec;pAVStream = pAVFormatContext->streams[index];break;}
    }//提取结束
	VideoIndex=videoIndex;
	if(videoIndex == -1 || !vCodecCtx){cout << "Failed to find video stream";return;}//判断是否找到视频流
	
    // 步骤五：对找到的视频流寻解码器
    vCodec = avcodec_find_decoder(vCodecCtx->codec_id);
    if(!vCodec){cout<<"Failed to avcodec_find_decoder(vCodecCtx->codec_id):"<< vCodecCtx->codec_id;return; }

    // 步骤六：打开解码器    
    av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);// 设置缓存大小 1024000byte    
    av_dict_set(&pAVDictionary, "stimeout", "20000000", 0);// 设置超时时间 20s    
    av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);// 设置最大延时 3s    
    av_dict_set(&pAVDictionary, "rtsp_transport", "tcp", 0);// 设置打开方式 tcp/udp
    if(avcodec_open2(vCodecCtx, vCodec, &pAVDictionary)){cout << "Failed to avcodec_open2(vCodecCtx, vCodec, pAVDictionary)";return;}
    
    // 显示视频相关的参数信息（编码上下文）                            
    cout<<"比特率:"		<<vCodecCtx->bit_rate			<<endl;
    cout<<"总时长:"		<<pAVStream->duration/10000.0	<<"秒\n";
    cout<<"总帧数:"		<<pAVStream->nb_frames			<<endl;	
    cout<<"格式:"		<<vCodecCtx->pix_fmt			<<endl;//格式为0说明是AV_PIX_FMT_YUV420P
    cout<<"宽:"			<<vCodecCtx->width				<<"\t高："		<<vCodecCtx->height				<<endl;
	cout<<"文件分母:"	<<vCodecCtx->time_base.den		<<"\t文件分子:"	<<vCodecCtx->time_base.num		<<endl;
    cout<<"帧率分母:"	<<pAVStream->avg_frame_rate.den	<<"\t帧率分子:"	<<pAVStream->avg_frame_rate.num	<<endl;
    // 有总时长的时候计算帧率（较为准确）
	double fps=pAVStream->avg_frame_rate.num * 1.0f / pAVStream->avg_frame_rate.den;if(fps<=0){cout<<fps<<endl;return;}//帧率
    double interval = 1 * 1000 / fps;//帧间隔
    cout<<"平均帧率:"	<< fps<<endl;
    cout<<"帧间隔:"		<< interval << "ms"<<endl;

    // 步骤七：对拿到的原始数据格式进行缩放转换为指定的格式高宽大小  AV_PIX_FMT_YUV420P        vCodecCtx->pix_fmt AV_PIX_FMT_RGBA
    pSwsContext = sws_getContext(vCodecCtx->width,vCodecCtx->height,vCodecCtx->pix_fmt,vCodecCtx->width,vCodecCtx->height,AV_PIX_FMT_YUV420P,SWS_FAST_BILINEAR,0,0,0);
    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P,vCodecCtx->width,vCodecCtx->height);
    outBuffer = (unsigned char *)av_malloc(numBytes);
    avpicture_fill((AVPicture *)pAVFrameRGB32,outBuffer,AV_PIX_FMT_YUV420P,vCodecCtx->width,vCodecCtx->height);//pAVFrame32的data指针指向了outBuffer

	//---------------------------------------------------------SDL相关变量预先定义----------------------------------------------------------------
    SDL_Window *pSDLWindow = 0;
    SDL_Renderer *pSDLRenderer = 0;
    SDL_Surface *pSDLSurface = 0;
    SDL_Texture *pSDLTexture = 0;
    SDL_Event event;

    if(SDL_Init(SDL_INIT_VIDEO| SDL_INIT_AUDIO | SDL_INIT_TIMER))//初始化SDL 
		{cout<<"Failed SDL_Init\n";				return;}
	int set_width,set_height;
	if(vCodecCtx->width>my_width)set_width=my_width;else set_width=vCodecCtx->width;
	if(vCodecCtx->height>my_height) set_height=my_height;else set_height=vCodecCtx->height;
	pSDLWindow = SDL_CreateWindow("ZasLeonPlayer",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,set_width,set_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);//设置显示框大小
    if(!pSDLWindow)
		{cout<<"Failed SDL_CreateWindow\n";		return;}
    pSDLRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0);
	if(!pSDLRenderer)
		{cout<<"Failed SDL_CreateRenderer\n";	return;}
    pSDLTexture = SDL_CreateTexture(pSDLRenderer,SDL_PIXELFORMAT_YV12,SDL_TEXTUREACCESS_STREAMING,vCodecCtx->width,vCodecCtx->height);
	if(!pSDLTexture)
		{cout<<"Failed SDL_CreateTexture\n";	return;}
	//---------------------------------------------------------SDL定义结束------------------------------------------------------------------------

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)process_audio_function, NULL, NULL, NULL);//开始音频线程部署并播放
   
	// 步骤八：读取一帧数据的数据包
    while(av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
		if(pAVPacket->stream_index ==AudioIndex)video_pace++;
		
		GetLocalTime(&startTime);//获取解码前精确时间
        if(pAVPacket->stream_index == videoIndex)//如果该帧为视频帧，视频处理开始
        {
			ret=avcodec_send_packet(vCodecCtx, pAVPacket);
            if(ret){cout << "Failed to avcodec_send_packet(vCodecCtx, pAVPacket) ,ret =" << ret;break;}
			
            while(!avcodec_receive_frame(vCodecCtx, pAVFrame))
            {
                sws_scale(pSwsContext,(const uint8_t * const *)pAVFrame->data,pAVFrame->linesize,0,vCodecCtx->height,pAVFrameRGB32->data,pAVFrameRGB32->linesize);

                SDL_UpdateYUVTexture(pSDLTexture,NULL,pAVFrame->data[0], pAVFrame->linesize[0],pAVFrame->data[1], pAVFrame->linesize[1],pAVFrame->data[2], pAVFrame->linesize[2]);

                SDL_RenderClear(pSDLRenderer);
                // Texture复制到Renderer
                SDL_Rect sdlRect;
                sdlRect.x=0;
				sdlRect.y=0;
				if(pAVFrame->width>my_width)sdlRect.w=my_width;else sdlRect.w=pAVFrame->width;
				if(pAVFrame->height>my_height)sdlRect.h = my_height;else sdlRect.h = pAVFrame->height;
				
                SDL_RenderCopy(pSDLRenderer, pSDLTexture, 0, &sdlRect) ;
                SDL_RenderPresent(pSDLRenderer);// 更新Renderer显示
                SDL_PollEvent(&event);// 事件处理
            }   
			if(output_over)//如果音频输出结束，图片按图片帧时间进行等待后刷新播放
			{
				GetLocalTime(&nextTime);//获取解码后精确时间
				if(nextTime.wMilliseconds<startTime.wMilliseconds)nextTime.wMilliseconds=nextTime.wMilliseconds+1000;//如果解码后毫秒小于解码前，解码后时间+1000毫秒补
				int delaytime=int(interval)-int(nextTime.wMilliseconds-startTime.wMilliseconds);//计算解码前后毫秒差
				if(delaytime<interval&&delaytime>0)Sleep(delaytime);//正常延迟，防止播放速度过快，这里故意delaytime-1让画面快一点刷新，靠下面的死循环补正同步
			}
			while(video_pace>audio_pace){if(video_pace==audio_pace)break;}//防止话音不同步，靠死循环等待来同步，这里故意设置让声音稍微比画面快一帧audio_pace+1
        }//一帧视频处理结束

    }//视频解码彻底完成，死循环结束
	cout<<"视频解码结束\n";
	while(!output_over){SDL_PollEvent(&event);Sleep(30);}//如果音频没输出完，进行等待
	cout << "释放回收资源,关闭SDL"<<endl;
	Sleep(500);
	if(outBuffer){av_free(outBuffer);outBuffer = 0;}
	if(pSwsContext){sws_freeContext(pSwsContext);pSwsContext = 0;}
	if(pAVFrameRGB32){av_frame_free(&pAVFrameRGB32);pAVFrame = 0;}
	if(pAVFrame){av_frame_free(&pAVFrame);pAVFrame = 0;} 
    if(pAVPacket){av_free_packet(pAVPacket);pAVPacket = 0;}
    if(vCodecCtx){avcodec_close(vCodecCtx);vCodecCtx = 0;}
    if(pAVFormatContext){avformat_close_input(&pAVFormatContext);avformat_free_context(pAVFormatContext);pAVFormatContext = 0;}
	
    // 步骤五：销毁渲染器
    SDL_DestroyRenderer(pSDLRenderer);
    // 步骤六：销毁窗口
    SDL_DestroyWindow(pSDLWindow);cout<<"即将执行SDL_Quit()\n";
    // 步骤七：退出SDL
    SDL_Quit();
	
}