#include "process_video.h"

#undef main;
int main()
{
	int k;
	if(av_alive)
		show_moive_alive();//直播TS流
	else
		show_moive();//本地影片
	cout<<"彻底结束\n";
	cin>>k;

}