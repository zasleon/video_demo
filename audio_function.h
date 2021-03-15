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
#define vod			1//�㲥
#define liveshow	2//�ֳ�ֱ��
#define local		3//����ӰƬ

//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\ɱ¾���У�OBD��������1280����[www.66ys.tv].rmvb";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\���Ƕ��ǳ������ߣ�.mp4";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Ӣ������ .mp3";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";int movie_type=local;
char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\hc\\123123.mp4";int movie_type=local;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\don diablo.mp4";int movie_type=local;
//char rtspUrl[] = "123123.mp4";int movie_type=local;
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dfhd.m3u8";int movie_type=liveshow;//����
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dftv.m3u8";int movie_type=liveshow;
//char rtspUrl[] = "https://vip.w.xk.miui.com/10d7ff4c9368d411b254bb5c62d6e849";int movie_type=vod;
//http://1251316161.vod2.myqcloud.com/5f6ddb64vodsh1251316161/c841de715285890814995493210/ALat0G5iwIQA.mp4
//char rtspUrl[] = "https://s1.cdn-c55291f64e9b0e3a.com/common/maomi/mm_m3u8_online/ny_WaG7EVh5/hls/1/index.m3u8?auth_key=1615797185-123-0-eb20392131a0559921ac22d5365bc869";int movie_type=vod;

#define SDL_AUDIO_BUFFER_SIZE 1024
const int my_width=1280;//�Լ����Էֱ��ʣ�����������ʾʱ��������
const int my_height=720;
static int set_width,set_height;
//---------------------------------------------------����Ƶ������߼�����----------------------------

const int bufnumber=2000;//������Ƶ�ڴ���������Ƶ֡������
const int single_buf_size=65535*2;//��֡��Ƶ���������
static bool next_write,next_read;//��Ƶд���ڴ������Լ����ˣ�д������һ����д�굱ǰ�ڴ�飬���еȴ������Ź��ܷ��͸�ָ��next_write���������д��һ���ˣ���Ž�����һ���ڴ���д�룩
static bool audio_output_over;//��Ƶ���������
static double video_pace,audio_pace,decode_audio_pace,decode_video_pace;//ȷ�ϵ�ǰ���ȣ���֤��Ƶ��Ƶͬ��
static int AudioIndex,VideoIndex;//ȫ��ȷ�ϸ�֡�Ƿ�����Ƶ֡����Ƶ֡
bool write_audio_buff_over;//��Ƶ�Ƿ������trueΪ������falseΪû����
static int audio_framesize,audio_fre;//��֡��Ƶ���ݴ�С,��Ƶ������
struct Audio_buf
{
	uint8_t audio_info[single_buf_size]; 
	int bufsize;//��֡��Ƶ��С
	int write_buf_index;//д��ƫ����
	int read_buf_index;//����ƫ����
	double pts;
	double gap;//�ͺ�һʱ��Ĳ��
};
static Audio_buf audio_buf[bufnumber];
static Audio_buf* read_this_buf;
static Audio_buf* write_this_buf;
static int write_count,read_count;//��¼��ǰ�ڶ���д���ڴ��
