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
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\ɱ¾���У�OBD��������1280����[www.66ys.tv].rmvb";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\btest\\Wildlife.wmv";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\���Ƕ��ǳ������ߣ�.mp4";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Ӣ������ .mp3";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\Kalimba.mp3";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\hc\\123123.mp4";bool av_alive=false;
//char rtspUrl[] = "F:\\JY\\www.NewXing.com\\moive\\don diablo.mp4";bool av_alive=false;
//char rtspUrl[] = "123123.mp4";bool av_alive=false;
//char rtspUrl[] = "http://ivi.bupt.edu.cn/hls/dfhd.m3u8";bool av_alive=true;//����
char rtspUrl[] = "https://www.ibilibili.com/video/BV14f4y147yq";bool av_alive=true;
//char rtspUrl[] = "https://vip.w.xk.miui.com/10d7ff4c9368d411b254bb5c62d6e849";bool av_alive=true;
//http://1251316161.vod2.myqcloud.com/5f6ddb64vodsh1251316161/c841de715285890814995493210/ALat0G5iwIQA.mp4
//char rtspUrl[] = "https://s1.cdn-c55291f64e9b0e3a.com/common/maomi/mm_m3u8_online/wm_3bJY6KKJ/hls/1/index.m3u8?auth_key=1615372831-123-0-8bf32eec501ee45f5d50a45824124844";bool av_alive=true;
//---------------------------------------------------ffmpeg���ý���----------------------------
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
	audio_pace=-1;decode_audio_pace=-1;
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
	bool first_valid=false;

	AVPacket *pAPacket = av_packet_alloc();							// ffmpag��֡���ݰ�
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
				cout<<data_size<<endl;
				write_this_buf->bufsize=data_size;
				//cout<<double(frame->best_effort_timestamp)<<"\t"<<(av_q2d(pAFormatContext->streams[audioIndex]->time_base))<<endl;
				//cout<<double(frame->best_effort_timestamp)*(av_q2d(pAFormatContext->streams[audioIndex]->time_base))<<endl;
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