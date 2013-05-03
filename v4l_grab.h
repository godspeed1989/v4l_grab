#ifndef __V4LGRAB_H__
#define __V4LGRAB_H__

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct buffer
{
	void *start;
	u32 length;
};

typedef class v4l_grab
{
public:
	int camera_fd;
	const char* dev;
	u32 width;
	u32 height;
	u32 n_buffer;
	struct buffer *buffers;
	u8 *frame_buffer; // size is width*height*3
	v4l_grab(const char* device, u32 w, u32 h, u32 n_buffers);
	~v4l_grab();
	int init_camera();
	void close_camera();
	int get_next_frame();
	int release_frame();
	void fb_yuv_2_rgb();
	int save_bmp(const char* filename);
	int save_yuv(const char* filename);
private:
	int index;
	int init_buffer();
}v4l_grab;

#endif //__V4LGRAB_H___

