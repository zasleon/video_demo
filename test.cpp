#include "local_stream.h"

#undef main;
int main()
{
	int k;
	switch(movie_type)
	{
		case liveshow:show_moive_alive();//直播TS流
		case local: show_moive();//本地影片
		case vod:show_moive_vod();//网络点播
	}
	cout<<"彻底结束\n";
	cin>>k;

}