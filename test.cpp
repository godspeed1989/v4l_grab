#include "v4l_grab.h"

#define VIDEO_DEV   "/dev/video0"

#define BMP         "./image_bmp.bmp"
#define YUV         "./image_yuv.yuv"

int main(void)
{
	v4l_grab v4lgrab(VIDEO_DEV, 640, 480, 4);
	if(v4lgrab.init_camera())
	{
		return -1;
	}
	v4lgrab.grab();
	v4lgrab.save_bmp(BMP);
	v4lgrab.save_yuv(YUV);
	v4lgrab.close_camera();

	return 0;
}

