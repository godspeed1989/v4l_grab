#include "v4l_grab.h"
#include <unistd.h>
#include <cstdio>

#define VIDEO_DEV   "/dev/video0"

static char BMP[64];
static char YUV[64];

int main(void)
{
	v4l_grab v4lgrab(VIDEO_DEV, 640, 480, 5);
	if(v4lgrab.init_camera())
	{
		return -1;
	}
	for(int i=0; i<10; i++)
	{
		sprintf(BMP, "./image%d.bmp", i);
		sprintf(YUV, "./image%d.yuv", i);
		v4lgrab.get_next_frame();
		v4lgrab.fb_yuv_2_rgb();
		v4lgrab.save_bmp(BMP);
		v4lgrab.save_yuv(YUV);
		v4lgrab.release_frame();
		usleep(500*1000);
	}
	v4lgrab.close_camera();
	return 0;
}

