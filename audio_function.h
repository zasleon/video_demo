#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
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
using namespace std;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\杀戮都市：OBD日语中字1280高清[www.66ys.tv].rmvb";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\我们都是超能力者！.mp4";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\英雄联盟 .mp3";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\hc\\123123.mp4";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\don diablo.mp4";bool av_alive=false;
//char rtspUrl[] = "123123.mp4";bool av_alive=false;
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dfhd.m3u8";bool av_alive=true;//高清
char rtspUrl[] = "https://www.ibilibili.com/video/BV14f4y147yq";bool av_alive=true;
//char rtspUrl[] = "https://vip.w.xk.miui.com/10d7ff4c9368d411b254bb5c62d6e849";bool av_alive=true;
//http://1251316161.vod2.myqcloud.com/5f6ddb64vodsh1251316161/c841de715285890814995493210/ALat0G5iwIQA.mp4
//char rtspUrl[] = "https://s1.cdn-c55291f64e9b0e3a.com/common/maomi/mm_m3u8_online/wm_3bJY6KKJ/hls/1/index.m3u8?auth_key=1615372831-123-0-8bf32eec501ee45f5d50a45824124844";bool av_alive=true;
//---------------------------------------------------ffmpeg配置结束----------------------------
#define SDL_AUDIO_BUFFER_SIZE 1024
const int my_width=1280;//自己电脑分辨率，超过则在显示时进行限制
const int my_height=720;
static int set_width,set_height;
//---------------------------------------------------对音频输出的逻辑控制----------------------------

const int bufnumber=2000;//设置音频内存块个数，视频帧包个数
const int single_buf_size=65535*2;//单帧音频的最大容量
static bool next_write,next_read;//音频写入内存服务可以继续了（写服务是一下子写完当前内存块，进行等待。播放功能发送该指令next_write（你可以先写下一块了）后才进行下一块内存块的写入）
static bool audio_output_over;//音频输出结束了
static double video_pace,audio_pace,decode_audio_pace,decode_video_pace;//确认当前进度，保证视频音频同步
static int AudioIndex,VideoIndex;//全局确认该帧是否是音频帧或视频帧
bool write_audio_buff_over;//音频是否结束，true为结束，false为没结束
static int audio_framesize,audio_fre;//单帧音频数据大小,音频采样率
struct Audio_buf
{
	uint8_t audio_info[single_buf_size]; 
	int bufsize;//该帧音频大小
	int write_buf_index;//写块偏移量
	int read_buf_index;//读块偏移量
	double pts;
	double gap;//和后一时间的差额
};
static Audio_buf audio_buf[bufnumber];
static Audio_buf* read_this_buf;
static Audio_buf* write_this_buf;
static int write_count,read_count;//记录当前在读或写的内存块
//---------------------------------------------------对音频输出的逻辑结束----------------------------
void audio_callback(void *userdata, Uint8 *stream, int len) //音频回调函数，SDL_OpenAudio使用后会无限循环使用该函数，直到使用SDL_closeAudio，如果不close则SDL_Quit无限阻塞
{
	
	if(audio_output_over&&read_this_buf->bufsize==0)return;//如果宣布结束了,且音频播放完了,返回

	while(read_this_buf->bufsize<=0)Sleep(100);//如果该缓冲为空，等待

	audio_pace=read_this_buf->pts+double(read_this_buf->gap)*(double(read_this_buf->read_buf_index)/double(read_this_buf->bufsize));////更新音频播放到的时间

	//memcpy(stream, audiobuf[read_count]+audio_buf_read_index[read_count], len);//将自己携带信息进行播放,据说memcpy这样直接做会造成失真，用以下两句代替SDL_memset,SDL_MixAudio
	SDL_memset(stream, 0, len);
	if(len+read_this_buf->read_buf_index>single_buf_size){cout<<"炸了\n";audio_output_over=true;return;}
	if((len+read_this_buf->read_buf_index)<=read_this_buf->bufsize)
	{
		if((read_count==(bufnumber/2+1)||read_count==1)&&read_this_buf->read_buf_index<len)next_write=true;//到达固定地点，发送“解码缓冲开启，继续写”信号

		SDL_MixAudio(stream,read_this_buf->audio_info+read_this_buf->read_buf_index,len,SDL_MIX_MAXVOLUME);//核心功能句，将帧中解析出来的数据流输入stream并输出
		read_this_buf->read_buf_index+= len;//确定下一段要播放的信息
		if(read_this_buf->read_buf_index==read_this_buf->bufsize)//如果刚好读完，切换内存块
		{
			read_this_buf->bufsize=0;
			read_count++;if(read_count>=bufnumber)read_count=0;//如果已经是最大内存块了，重新读取最初的内存块
			read_this_buf=&audio_buf[read_count];
			read_this_buf->read_buf_index=0;
		}
	}
	else
	{
		if(len>=single_buf_size){cout<<"大于single_buf_size\n";return;}
		uint8_t temp[single_buf_size*2]={0};//创建临时缓存块
		int remain_size=read_this_buf->bufsize-read_this_buf->read_buf_index;//确认当前读取的缓存块剩余大小
		if(remain_size<0){cout<<"remain_size"<<remain_size<<" read_this_buf->read_buf_index"<<read_this_buf->read_buf_index<<" read_this_buf->bufsize"<<read_this_buf->bufsize<<endl;Sleep(500);return;}
		memcpy(temp,read_this_buf->audio_info+read_this_buf->read_buf_index,remain_size);//将剩余部分内容copy到新创建的临时缓存块里
		read_this_buf->bufsize=0;
		read_this_buf->read_buf_index=0;//该缓存块写入偏移量置0，写服务切换内存块后，切换前的内存块写入偏移量不置0，在这里置0
		read_count++;if(read_count>=bufnumber)read_count=0;//如果已经是最大内存块了，重新读取最初的内存块
		read_this_buf=&audio_buf[read_count];
		memcpy(temp+remain_size,read_this_buf->audio_info,len-remain_size);//读取新内存块的剩余部分量，即len-remain_size
		SDL_MixAudio(stream,temp,len,SDL_MIX_MAXVOLUME);//将临时缓存块的内容输入音频输出流
		if(write_audio_buff_over&&read_this_buf->write_buf_index==0){audio_output_over=true;cout<<"结束了\n";}//如果音频没帧了且下一块内存内容为空，宣布输出彻底结束output_over=true
		read_this_buf->read_buf_index= len-remain_size;//确定下一段要播放的信息
	}
}

void process_audio_function()
{
	cout<<"开始分析文件音频\n";
		//--------------------------------------------------音频控制参数初始化------------------------------------------------------------------
	audio_pace=-1;decode_audio_pace=-1;
	AudioIndex=-1;//初始没找到音频检索
	write_audio_buff_over=false;//当前音频输出没有结束
	next_write=false;next_read=false;//当前不切换内存写入音频数据
	audio_output_over=false;//当前输出等待列队没输出完
	for(int i=0;i<bufnumber;i++)
	{
		memset(audio_buf[i].audio_info,0,sizeof(audio_buf[i].audio_info));//清空所有内存块内容
		audio_buf[i].read_buf_index=0;audio_buf[i].write_buf_index=0;//当前读写内存块偏移量置0
		audio_buf[i].bufsize=0;audio_buf[i].pts=0;//dts和内存块写入大小置为0
	}
	write_this_buf=&audio_buf[0];
	read_this_buf=&audio_buf[0];
	read_count=0;write_count=0;//音频的读写内存块都设置为第一块，往第一块里读，往第一块里写
	//------------------------------------------------音频控制参数初始化结束-----------------------------------------------------------------

	int ret=0;//储存ffmpeg操作发来的错误码
	char buf[1024]={0};//储存ffmpeg操作发来的错误信息
	
	AVFormatContext* pAFormatContext = avformat_alloc_context();	// 重新分配上下文，貌似无法与主线程中解析视频帧的上下文共用

	// 打开文件(ffmpeg成功则返回0),绑定pAFormatContext和rtspUrl
    if(avformat_open_input(&pAFormatContext,rtspUrl, NULL, NULL)){av_strerror(ret, buf, 1024);cout<<"打开失败,错误编号:"<<ret;return;}
	
	cout<<"-------------------------------------ffmpeg分析信息-----------------------\n";
	av_dump_format(pAFormatContext, 0, rtspUrl, 0);
	cout<<"-------------------------------------ffmpeg分析结束-----------------------\n";

	//int audioIndex= av_find_best_stream(pAFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);if(audioIndex<0){cout<<audioIndex<<"无音频\n";return;}
	for(int index = 0; index < pAFormatContext->nb_streams; index++)
    { 
        switch (pAFormatContext->streams[index]->codec->codec_type)
        {
			case AVMEDIA_TYPE_UNKNOWN:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_UNKNOWN\n";break;
			case AVMEDIA_TYPE_VIDEO:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_VIDEO\n";break;
			case AVMEDIA_TYPE_AUDIO:AudioIndex = index;	cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_AUDIO\n";break;
			case AVMEDIA_TYPE_DATA:						cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_DATA\n";break;
			case AVMEDIA_TYPE_SUBTITLE:					cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_SUBTITLE\n";break;
			case AVMEDIA_TYPE_ATTACHMENT:				cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_ATTACHMENT\n";break;
			case AVMEDIA_TYPE_NB:						cout << "流序号:" << index << "类型为:AVMEDIA_TYPE_NB\n";break;
			default:break;
        }
        if(AudioIndex >0)// 已经找打视频品流,跳出循环
		{break;}
    }//提取结束
	if(AudioIndex<0){cout<<"无音频\n";write_audio_buff_over=true;audio_output_over=true;return;}

	AVCodecContext *pAudioCodecCtx =avcodec_alloc_context3(NULL);//此两句话可用老版本概括：AVCodecContext *pAudioCodecCtx = pAFormatContext->streams[audioIndex]->codec;
	avcodec_parameters_to_context(pAudioCodecCtx,pAFormatContext->streams[AudioIndex]->codecpar);
	AVStream *pAStream = pAFormatContext->streams[AudioIndex];
    AVCodec *pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);//找到适合的解码器
    pAudioCodecCtx->pkt_timebase=pAFormatContext->streams[AudioIndex]->time_base;//不加这句，会出现：mp3float：Could not update timestamps for skipped samples
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
	//--------------------------------------------重采样初始化开始----------------------------------------------
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
	//------------------------------------------------重采样初始化结束------------------------------------------
	double last_best_effort_timestamp=0;//最后一次有效时间戳
	int lost_time=0;//丢失有效时间的帧的次数
	bool first_valid=false;

	AVPacket *pAPacket = av_packet_alloc();							// ffmpag单帧数据包
	while(av_read_frame(pAFormatContext, pAPacket) >= 0)//当文件内容没有到头时，解包分析
    {
		if(pAPacket->stream_index == AudioIndex)//如果当前包内信息为音频信息，解码，不是音频则忽视，经过等待后进行下次解包
		{
			avcodec_send_packet(pAudioCodecCtx, pAPacket);
			AVFrame *frame = av_frame_alloc();//分配帧空间
			while(avcodec_receive_frame(pAudioCodecCtx, frame) >= 0)//解码开始
			{
				int uSampleSize = 4;
				int data_size = uSampleSize * pAudioCodecCtx->channels * frame->nb_samples;//确认播放信息字节数大小
				audio_framesize=data_size;
				cout<<data_size<<endl;
				write_this_buf->bufsize=data_size;
				//cout<<double(frame->best_effort_timestamp)<<"\t"<<(av_q2d(pAFormatContext->streams[audioIndex]->time_base))<<endl;
				//cout<<double(frame->best_effort_timestamp)*(av_q2d(pAFormatContext->streams[audioIndex]->time_base))<<endl;
				if(frame->best_effort_timestamp>0)//可能存在帧时间戳丢失的情况（该值为负值），比如直播，每x帧才有一个有效时间戳，拿此来填补之前损失的
				{
					write_this_buf->pts=double(frame->best_effort_timestamp)* av_q2d(pAFormatContext->streams[AudioIndex]->time_base);//确定当前有效时间戳
					double gap=(double(write_this_buf->pts)-double(last_best_effort_timestamp))/double(lost_time+1);//计算与之前那次有效时间戳的两帧间差距,并计算和前一帧时间差距
					write_this_buf->gap=gap;//写入该帧时间差
					last_best_effort_timestamp=write_this_buf->pts;//更新最后一次有效时间戳
					decode_audio_pace=write_this_buf->pts;//确认读到哪一帧了

					int k=write_count-1;if(k<0)k=bufnumber-1;//开始往前几帧无时间戳的帧填补pts和gap
					double this_frame_timestamp=last_best_effort_timestamp;//time_fill为此帧时间戳，则前一帧时间戳为time_fill-gap
					while(audio_buf[k].gap==0&&lost_time>0)
					{
						audio_buf[k].pts=double(this_frame_timestamp)-double(gap);//赋予前一帧时间戳
						audio_buf[k].gap=double(gap);//赋予前一帧时间差距
						this_frame_timestamp-=double(gap);//定位此帧时间戳，下个循环再往前一帧补
						lost_time--;//累计丢失量-1，如果丢失量为0了退出循环
						k--;if(k<0)k=bufnumber-1;
					}
					lost_time=0;//丢失量清0
				}
				else//如果该帧时间戳错误，变为最后一帧正常的时间戳
				{
					lost_time++;//丢失量+1
					write_this_buf->pts=last_best_effort_timestamp;//先将当前时间戳补为最近一次有效时间戳
					write_this_buf->gap=0;//与后一帧时间差距为0
				}

				for(int i=0;i<frame->nb_samples;i++)//向音频缓存写入该帧内容
				{
					for(int j=0;j<pAudioCodecCtx->channels;j++)
					{
						if(write_this_buf->write_buf_index<write_this_buf->bufsize)//如果写入缓存有足够剩余
						{
							memcpy(write_this_buf->audio_info+write_this_buf->write_buf_index, frame->data[j] + uSampleSize * i, uSampleSize);//正常写入全局内存块，等待音频回调函数调用该内存块读取，回调函数读取后自动播放
							write_this_buf->write_buf_index +=uSampleSize;//缓存块写入偏移量增加
							
							if(write_this_buf->write_buf_index==write_this_buf->bufsize)//切换内存条
							{
								write_count++;if(write_count==bufnumber)write_count=0;
								write_this_buf=&audio_buf[write_count];
								write_this_buf->write_buf_index=0;//写入偏移量置0
								memset(write_this_buf->audio_info,0,sizeof(write_this_buf->audio_info));
								
								while(!next_write&&(write_count==bufnumber/2||write_count==bufnumber-1)){}//切换过缓存块了，等待回调函数返回next_write信号，返回后写满下一条缓存块

								if(write_count==bufnumber/2||write_count==bufnumber-1)
									{next_write=false;}//将"你可以开始写下一块内存块了"信号置否
							}
						}//else{cout<<"???"<<write_this_buf->write_buf_index<<"\t"<<write_this_buf->bufsize<<endl;Sleep(5000);}//理论上不会执行到这句话
					}
				
				}//音频帧内容写入缓存结束

				
			}//解码packet完成

			
		}//一帧音频处理结束
		
    }//解码彻底完成，整个文件读完，死循环结束
	if(write_count-1==-1)//计算最后一帧的差距时间
		write_this_buf->gap=audio_buf[bufnumber-1].gap;
	else
		write_this_buf->gap=audio_buf[write_count-1].gap;

	write_audio_buff_over=true;//音频解码结束
	cout<<"音频写服务结束\n";
	while(!audio_output_over){}//如果音频回调输出还没结束
	cout<<"SDL_Audio关闭\n";
	SDL_CloseAudio();//音频关闭,不执行这句话，SDL_Quit将阻塞
}