#include "v4l_grab.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <linux/videodev2.h>


int v4l_grab::init_camera()
{
	static struct v4l2_capability cap;
	static struct v4l2_fmtdesc fmtdesc;
	static struct v4l2_format fmt;
	static struct v4l2_streamparm parm;
	if(camera_fd == -1)
	{
		if((camera_fd = open(dev, O_RDWR)) == -1)
		{
			printf("Error open device %s\n", dev);
			return -1;
		}
	}
	// query camera info
	if(ioctl(camera_fd, VIDIOC_QUERYCAP, &cap) == -1)
	{
		printf("Error query device %s\n", dev);
		return -1;
	}
	else
	{
		printf("driver:\t\t%s\n", cap.driver);
		printf("card:\t\t%s\n", cap.card);
		printf("bus_info:\t%s\n", cap.bus_info);
		printf("version:\t%d\n", cap.version);
		printf("capabilities:\t%x\n", cap.capabilities);
		if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
		{
			printf("Device %s: supports capture.\n", dev);
		}
		if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
		{
			printf("Device %s: supports streaming.\n", dev);
		}
	}
	// list all support fmt of camera
	fmtdesc.index = 0;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(camera_fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
	{
		printf("	%d.%s\n", fmtdesc.index+1, fmtdesc.description);
		fmtdesc.index++;
	}

	// setup capture fmt
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	if(ioctl(camera_fd, VIDIOC_S_FMT, &fmt) == -1)
	{
		printf("Unable to setup format\n");
		return -1;
	}
	if(ioctl(camera_fd, VIDIOC_G_FMT, &fmt) == -1)
	{
		printf("Unable to get format\n");
		return -1;
	}
	printf("fmt.type:\t\t%d\n", fmt.type);
	printf("pix.pixelformat:\t%c%c%c%c\n",
			fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
			(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
	printf("pix.height:\t\t%d\n", fmt.fmt.pix.height);
	printf("pix.width:\t\t%d\n", fmt.fmt.pix.width);
	printf("pix.field:\t\t%d\n", fmt.fmt.pix.field);

	//set fps
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 10;
	parm.parm.capture.timeperframe.denominator = 10;
	ioctl(camera_fd, VIDIOC_S_PARM, &parm);
	printf("Init %s \t[OK]\n", dev);

	return 0;
}

int v4l_grab::grab()
{
	u32 i_buf;
	// request for 4 buffers
	static struct v4l2_requestbuffers req;
	static struct v4l2_buffer buf;
	req.count = n_frame;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(camera_fd, VIDIOC_REQBUFS, &req) == -1)
	{
		printf("request for buffers error\n");
	}

	// mmap buffers
	for (i_buf = 0; i_buf < req.count; i_buf++)
	{
		buf.index = i_buf;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		// query buffers
		if (ioctl(camera_fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			printf("query buffer %d error\n", i_buf);
			return -1;
		}

		buffers[i_buf].length = buf.length;
		// map
		buffers[i_buf].start =
			mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, camera_fd, buf.m.offset);
		if (buffers[i_buf].start == MAP_FAILED)
		{
			printf("buffer map error\n");
			return -1;
		}
	}

	//TODO: queue???
	for (i_buf = 0; i_buf < req.count; i_buf++)
	{
		buf.index = i_buf;
		ioctl(camera_fd, VIDIOC_QBUF, &buf);
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (camera_fd, VIDIOC_STREAMON, &type);

	ioctl(camera_fd, VIDIOC_DQBUF, &buf);

	printf("grab yuyv OK\n");
	return 0;
}

void v4l_grab::close_camera()
{
	if(camera_fd != -1)
	{
		close(camera_fd);
	}
}

static void yuyv_2_rgb888(u8 * pointer, u32 width, u32 height, u8* frame_buffer)
{
	u32 i, j;
	u8 y1, y2, u, v;
	int r1, g1, b1;
	int r2, g2, b2;
	width = width / 2;
	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			y1 = *( pointer + (i*width+j)*4);
			u  = *( pointer + (i*width+j)*4 + 1);
			y2 = *( pointer + (i*width+j)*4 + 2);
			v  = *( pointer + (i*width+j)*4 + 3);
    		
			r1 = y1 + 1.042*(v-128);
			g1 = y1 - 0.34414*(u-128) - 0.71414*(v-128);
			b1 = y1 + 1.772*(u-128);
    		
			r2 = y2 + 1.042*(v-128);
			g2 = y2 - 0.34414*(u-128) - 0.71414*(v-128);
			b2 = y2 + 1.772*(u-128);
    		
			if(r1>255)    r1 = 255;
			else if(r1<0) r1 = 0;
    		
			if(b1>255)    b1 = 255;
			else if(b1<0) b1 = 0;
    		
			if(g1>255)    g1 = 255;
			else if(g1<0) g1 = 0;

			if(r2>255)    r2 = 255;
			else if(r2<0) r2 = 0;
    		
			if(b2>255)    b2 = 255;
			else if(b2<0) b2 = 0;
    		
			if(g2>255)    g2 = 255;
			else if(g2<0) g2 = 0;
    			
			*(frame_buffer + ((height-1-i)*width+j)*6    ) = (u8)b1;
			*(frame_buffer + ((height-1-i)*width+j)*6 + 1) = (u8)g1;
			*(frame_buffer + ((height-1-i)*width+j)*6 + 2) = (u8)r1;
			*(frame_buffer + ((height-1-i)*width+j)*6 + 3) = (u8)b2;
			*(frame_buffer + ((height-1-i)*width+j)*6 + 4) = (u8)g2;
			*(frame_buffer + ((height-1-i)*width+j)*6 + 5) = (u8)r2;
		}
	}
}

typedef struct BITMAPFILEHEADER
{
	u16	bfType;        // the flag of bmp, value is "BM"
	u32 bfSize;       // size of BMP file
	u32 bfReserved;   // 0
	u32 bfOffBits;    // must be 54
}__attribute__((packed)) BITMAPFILEHEADER;
typedef struct BITMAPINFOHEADER
{
	u32 biSize;      // must be 0x28
	u32 biWidth;
	u32 biHeight;
	u16 biPlanes;    // must be 1
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;
	u32 biXPelsPerMeter;
	u32 biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
}__attribute__((packed)) BITMAPINFOHEADER;
typedef struct RGBQUAD
{
	u8 rgbBlue;
	u8 rgbGreen;
	u8 rgbRed;
	u8 rgbReserved;
}__attribute__((packed)) RGBQUAD;

int v4l_grab::save_bmp(const char* filename)
{
	FILE *fp;
	static BITMAPFILEHEADER bf;
	static BITMAPINFOHEADER bi;
	fp = fopen(filename, "wb");
	if(!fp)
	{
		printf("open %s error\n", filename);
		return -1;
	}
	// Setup BITMAPINFOHEADER
	bi.biSize = 40;
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = 0;
	bi.biSizeImage = width*height*3;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// Setup BITMAPFILEHEADER
	bf.bfType = 0x4d42;
	bf.bfSize = 54 + bi.biSizeImage;
	bf.bfReserved = 0;
	bf.bfOffBits = 54;
	
	yuyv_2_rgb888((u8*)buffers[0].start, width, height, frame_buffer);
	fwrite(&bf, 14, 1, fp);
	fwrite(&bi, 40, 1, fp);
	fwrite(frame_buffer, bi.biSizeImage, 1, fp);
	printf("save %s OK\n", filename);
	return 0;
}

int v4l_grab::save_yuv(const char* filename)
{
	FILE *fp;
	fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		printf("open %s error\n", filename);
		return -1;
	}
	
	fwrite(buffers[0].start, width*height*2, 1, fp);
	printf("save %s OK\n", filename);
	fclose(fp);
	return 0;
}

