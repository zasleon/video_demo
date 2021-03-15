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

void blank(){}
#define vod			1//点播
#define liveshow	2//现场直播
#define local		3//本地影片

//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\杀戮都市：OBD日语中字1280高清[www.66ys.tv].rmvb";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\我们都是超能力者！.mp4";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\英雄联盟 .mp3";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";int movie_type=local;
char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\hc\\123123.mp4";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\don diablo.mp4";int movie_type=local;
//char rtspUrl[] = "123123.mp4";int movie_type=local;
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dfhd.m3u8";int movie_type=liveshow;//高清
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dftv.m3u8";int movie_type=liveshow;
//char rtspUrl[] = "https://vip.w.xk.miui.com/10d7ff4c9368d411b254bb5c62d6e849";int movie_type=vod;
//http://1251316161.vod2.myqcloud.com/5f6ddb64vodsh1251316161/c841de715285890814995493210/ALat0G5iwIQA.mp4
//char rtspUrl[] = "https://s1.cdn-c55291f64e9b0e3a.com/common/maomi/mm_m3u8_online/ny_WaG7EVh5/hls/1/index.m3u8?auth_key=1615797185-123-0-eb20392131a0559921ac22d5365bc869";int movie_type=vod;

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
