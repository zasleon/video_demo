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

char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\杀戮都市：OBD日语中字1280高清[www.66ys.tv].rmvb";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\我们都是超能力者！.mp4";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\英雄联盟 .mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\hc\\123123.mp4";
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\don diablo.mp4";

const int my_width=1280;//自己电脑分辨率，超过则在显示时进行限制
const int my_height=800;

//---------------------------------------------------对音频输出的逻辑控制----------------------------
#define SDL_AUDIO_BUFFER_SIZE 1024
const int bufnumber=4;//设置内存块为4个
const int single_buf_size=4410000;//单块内存块的大小
static int audio_buf_read_index[bufnumber],audio_buf_write_index[bufnumber];//内存块内部读写偏移量
static uint8_t audiobuf[bufnumber][single_buf_size];//音频数据内存块
static int write_count,read_count;//记录当前在读或写的内存块
static bool next_write;//音频写入内存服务可以继续了（写服务是一下子写完当前内存块，进行等待。播放功能发送该指令next_write（你可以先写下一块了）后才进行下一块内存块的写入）
static bool output_over;//音频输出结束了
static int get_number,queque_number;//get_number获取队列号、queque_number当前队列号
static float video_pace,audio_pace;//确认当前进度，保证视频音频同步
static int AudioIndex,VideoIndex;//全局确认该帧是否是音频帧或视频帧
bool audio_over;//音频是否结束，true为结束，false为没结束
static int audio_framesize,audio_fre;//单帧音频数据大小,音频采样率

//---------------------------------------------------对音频输出的逻辑结束----------------------------

void audio_callback(void *userdata, Uint8 *stream, int len) //音频回调函数，SDL_OpenAudio使用后会无限循环使用该函数，直到使用SDL_closeAudio，如果不close则SDL_Quit无限阻塞
{
	
	int my_number=get_number;//从服务获取号码
	if(get_number==10000)//如果服务号码过大
		get_number=0;//服务号码重置为0
	else
		get_number++;//服务号码+1
	while(queque_number!=my_number){};//队列号码没轮到自己，进行等待
	if(output_over)return;//如果宣布结束了,返回
	
	audio_pace+=0.01;//float(len)/float(audio_framesize)*float(1024)/float(audio_fre);//更新音频播放到的时间


	int let_write=6;//在读取到内存块某处（let_write处）时，发送next_write指令让写入音频服务开始写入下一块内存块

	//memcpy(stream, audiobuf[read_count]+audio_buf_read_index, len);//将自己携带信息进行播放,据说memcpy这样直接做会造成失真，用以下两句代替SDL_memset,SDL_MixAudio
	SDL_memset(stream, 0, len);
	if(len+audio_buf_read_index[read_count]<=single_buf_size)//如果内存块有足够剩余（剩余量比len大）
	{
		SDL_MixAudio(stream,audiobuf[read_count]+audio_buf_read_index[read_count],len,SDL_MIX_MAXVOLUME);//核心功能句，将帧中解析出来的数据流输入stream并输出
		audio_buf_read_index[read_count]+= len;//确定下一段要播放的信息
	}
	else//内存块剩余量不足len长度了
	{
		next_write=true;//通知音频写入内存服务可以继续了
		if(len>50000)cout<<"大于50000\n";
		uint8_t temp[50000]={0};//创建临时缓存块
		int remain_size=single_buf_size-audio_buf_read_index[read_count];//确认当前读取的缓存块剩余大小
		memcpy(temp,audiobuf[read_count]+audio_buf_read_index[read_count],remain_size);//将剩余部分内容copy到新创建的临时缓存块里
		audio_buf_write_index[write_count]=0;//该缓存块写入偏移量置0，写服务切换内存块后，切换前的内存块写入偏移量不置0，在这里置0
		read_count++;if(read_count>=bufnumber)read_count=0;//如果已经是最大内存块了，重新读取最初的内存块
		memcpy(temp,audiobuf[read_count]+remain_size,len-remain_size);//读取新内存块的剩余部分量，即len-remain_size
		SDL_MixAudio(stream,temp,len,SDL_MIX_MAXVOLUME);//将临时缓存块的内容输入音频输出流
		cout<<read_count<<"切换读\n";
		if(audio_over&&audio_buf_write_index[read_count]==0){output_over=true;cout<<"结束了\n";}//如果音频没帧了且下一块内存内容为空，宣布输出彻底结束output_over=true
		audio_buf_read_index[read_count]= len-remain_size;//确定下一段要播放的信息
	}

	if(queque_number==10000)//如果列队号码过大
		{cout<<10000<<endl;queque_number=0;}//队列号码刷新为-1
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
	AVStream *pAStream = pAFormatContext->streams[audioIndex];
    AVCodec *pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);//找到适合的解码器
    pAudioCodecCtx->pkt_timebase=pAFormatContext->streams[audioIndex]->time_base;//不加这句，会出现：mp3float：Could not update timestamps for skipped samples
	assert(avcodec_open2(pAudioCodecCtx, pAudioCodec, nullptr) >= 0);
	
    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;
	cout<<"音频总帧数:"	<<pAStream->nb_frames<<endl;
	cout<<"音频总时长:"	<<pAStream->duration/10000.0	<<"秒\n";
	cout<<"音频采样率:"	<<pAudioCodecCtx->sample_rate	<<endl;
	cout<<"输出格式:"	<<AUDIO_F32SYS					<<endl;
	cout<<"声音通道数:"	<<pAudioCodecCtx->channels		<<endl;

	if(pAudioCodecCtx->sample_rate<=0)//mp3貌似pAudioCodecCtx->sample_rate都是0？导致输出的声音很慢很奇怪！
		desired_spec.freq = 44100;//该文件采样率数值的确为0，mp3的音频采样率默认设置为44100
	else
		desired_spec.freq = pAudioCodecCtx->sample_rate;//该文件有它自己的采样率,可以通过变化desired_spec.freq的值来改变播放速度,后面加上*x就是翻x倍
	audio_fre=desired_spec.freq;
    desired_spec.format = AUDIO_F32SYS;					
    desired_spec.channels = pAudioCodecCtx->channels;	
    desired_spec.silence = 0;
    desired_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    desired_spec.callback = audio_callback;//设置音频回调函数为audio_callback()
	CoInitialize(NULL);//SDL操作，线程无法直接使用主线程show_moive()中初始化后的SDL，加了这句才能使用主线程里初始化后的SDL，下面那句才不会报错
    assert(SDL_OpenAudio(&desired_spec, &obtained_spec) >= 0);//开启SDL音频播放
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
				audio_framesize=data_size;

				for(int i=0;i<frame->nb_samples;i++)
				{
					for(int j=0;j<pAudioCodecCtx->channels;j++)
					{
						if(audio_buf_write_index[write_count]+uSampleSize<=single_buf_size)//如果写入缓存有足够剩余
						{
							memcpy(audiobuf[write_count]+audio_buf_write_index[write_count], frame->data[j] + uSampleSize * i, uSampleSize);//正常写入全局内存块，等待音频回调函数调用该内存块读取，回调函数读取后自动播放
							audio_buf_write_index[write_count] +=uSampleSize;//缓存块写入偏移量增加
						}
						else//缓存剩余不足了，要切换缓存块进行写入
						{
							int remain_size=single_buf_size-audio_buf_write_index[write_count];//因为会超，所以将总容量减去偏移量得到剩余量，填满剩余量
							memcpy(audiobuf[write_count]+audio_buf_write_index[write_count], frame->data[j] + uSampleSize * i, remain_size);//先写满当前缓存块，没写入的等会儿写入
							write_count++;if(write_count>=bufnumber)write_count=0;//切换缓存块，如果缓存块到达上限，重置为第一块
							memset(audiobuf[write_count],0,sizeof(audiobuf[write_count]));//清空即将要写入的内存块的原有内容
							memcpy(audiobuf[write_count], frame->data[j] + uSampleSize * i+remain_size, uSampleSize-remain_size);//将剩余没写入的部分写入
							audio_buf_write_index[write_count]=uSampleSize-remain_size;//缓存块写入偏移量增加
							cout<<write_count<<"切换写\n";//打印切换记录
							while(!next_write){}//切换过缓存块了，等待回调函数返回next_write信号，返回后写满下一条缓存块
							next_write=false;//将“你可以开始写下一块内存块了”信号置否
						}
					}
				}
				
			}//解码完成
		}//一帧音频处理结束
			
			
    }//解码彻底完成，整个文件读完，死循环结束
	audio_over=true;//音频解码结束
	cout<<"总共为:"<<queque_number<<endl;
	while(!output_over){}//如果音频回调输出还没结束
	cout<<"SDL_Audio关闭\n";
	SDL_CloseAudio();//音频关闭,不执行这句话，SDL_Quit将阻塞
}

void show_moive()//读取本地视频
{
	video_pace=0;audio_pace=0;
	//--------------------------------------------------音频控制参数初始化------------------------------------------------------------------
	AudioIndex=-1;//初始没找到音频检索
	audio_over=false;//当前音频输出没有结束
	next_write=false;//当前不切换内存写入音频数据
	output_over=false;//当前输出等待列队没输出完
	for(int i=0;i<bufnumber;i++)memset(audiobuf[i],0,sizeof(audiobuf[i]));//清空所有内存块内容
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
	int VideoTimeStamp=0;				// 时间戳，防止视频没有自带的pts
	
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
			case AVMEDIA_TYPE_AUDIO:AudioIndex = index;	cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_AUDIO"<<endl;break;
			case AVMEDIA_TYPE_DATA:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_DATA"<<endl;break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_SUBTITLE"<<endl;break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_ATTACHMENT"<<endl;break;
			case AVMEDIA_TYPE_NB:						cout << "\n流序号:" << index << "类型为:" << "AVMEDIA_TYPE_NB"<<endl;break;
			default:break;
        }
        if(videoIndex != -1)// 已经找到视频品流,跳出循环
		{
			vCodecCtx = pAVFormatContext->streams[index]->codec;
			pAVStream = pAVFormatContext->streams[index];break;}
    }//提取结束

	VideoIndex=videoIndex;
	if(videoIndex == -1 || !vCodecCtx)
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
	
	av_init_packet(pAVPacket);
	// 步骤八：读取一帧数据的数据包
    while(av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
		//GetLocalTime(&startTime);//获取解码前精确时间
        if(pAVPacket->stream_index == videoIndex)//如果该帧为视频帧，视频处理开始
        {
			ret=avcodec_send_packet(vCodecCtx, pAVPacket);
            if(ret){cout<<"avcodec_send_packet失败!,错误码"<<ret;break;}
			
            while(!avcodec_receive_frame(vCodecCtx, pAVFrame))
            {
                sws_scale(pSwsContext,(const uint8_t * const *)pAVFrame->data,pAVFrame->linesize,0,vCodecCtx->height,pAVFrameRGB32->data,pAVFrameRGB32->linesize);

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
				//cout<<pAVFrame->pts<<"   "<<av_q2d(pAVStream->time_base)<<endl;
				if(pAVFrame->pts>0)//有些影片无总帧数无pts
					video_pace=pAVFrame->pts*av_q2d(pAVStream->time_base);//更新该帧的时间，实现音视频同步
				else
					video_pace+=interval/float(1000);
				//cout<<video_pace<<"   "<<audio_pace<<endl;
					
            }
			/*if(audio_over||video_pace>audio_pace)//原实现播放延迟代码
			{
				GetLocalTime(&nextTime);//获取解码后精确时间
				if(nextTime.wMilliseconds<startTime.wMilliseconds)nextTime.wMilliseconds=nextTime.wMilliseconds+1000;//如果解码后毫秒小于解码前，解码后时间+1000毫秒补
				int delaytime=int(interval)-int(nextTime.wMilliseconds-startTime.wMilliseconds);//计算解码前后毫秒差
				if(delaytime<interval&&delaytime>0)Sleep(delaytime);//正常延迟，防止播放速度过快，这里故意delaytime-1让画面快一点刷新，靠下面的死循环补正同步
			}
			else//画面比声音慢了*/

			while(video_pace>audio_pace&&!output_over){}//防止话音不同步，靠死循环等待来同步.这条语句代替了上面的“原实现播放延迟代码”
			if(output_over){Sleep(int(interval/float(1000)));cout<<"s"<<int(interval)<<endl;}
		

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