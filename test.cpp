#include "local_stream.h"

#undef main//�������ò�ƻ��sdl��main��ͻ����
void main()
{
	
	switch(movie_type)
	{
		case liveshow:show_moive_alive();//ֱ��TS��
		case local: show_moive();//����ӰƬ
		case vod:show_moive_vod();//����㲥
	}
	cout<<"���׽���\n";



	int k;cin>>k;
}