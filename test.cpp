#include "process_video.h"

#undef main;
int main()
{
	int k;
	if(av_alive)
		show_moive_alive();//ֱ��TS��
	else
		show_moive();//����ӰƬ
	cout<<"���׽���\n";
	cin>>k;

}