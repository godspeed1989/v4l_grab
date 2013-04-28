#ifndef __V4LGRAB_H__
#define __V4LGRAB_H__

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct buffer
{
	void *start;
	u32 length;
};

typedef struct v4l_grab
{
	int camera_fd;
	const char* dev;
	u32 width;
	u32 height;
	u32 n_frame;
	struct buffer *buffers;
	u8 *frame_buffer;
	v4l_grab(const char* device, u32 w, u32 h, u32 n_frames)
	{
		dev = device;
		width = w;
		height = h;
		camera_fd = -1;
		n_frame = n_frames;
		buffers = (buffer*)malloc(n_frame * sizeof(buffer));
		frame_buffer = (u8*)malloc(width * height * 3 * sizeof(u8));
		if (buffers == NULL || frame_buffer == NULL)
		{
			printf ("Out of memory\n");
			exit(-1);
		}
	}
	~v4l_grab()
	{
		free(buffers);
		free(frame_buffer);
	}
	int init_camera();
	void close_camera();
	int grab();
	int save_bmp(const char* filename, u32 idx);
	int save_yuv(const char* filename, u32 idx);
}v4l_grab;

#endif //__V4LGRAB_H___

