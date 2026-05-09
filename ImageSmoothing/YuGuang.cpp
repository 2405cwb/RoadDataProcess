// YuGuang.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <string>
#include "jpeglib.h"
#include <setjmp.h>
#include<iostream>
#include<fstream>
using namespace std;

int m_mode = 0;

BYTE    m_map1[256];
BYTE    m_mm[8192*256];
BYTE*   m_map2[8192];

int     m_ld=0;             //亮度
int     m_dbd=0;            //对比度
float   m_zm[8192]={0};     //照明度
int		m_tmpSumA[8192]={0};
int  picInfo = 1 ;	//cwb   picture passPassageway

#if 0
//20230218 cwb 扩大容量
unsigned char m_imgbuf[33554432];//8192*4096  //2168
unsigned char m_imgbufNew[33554432];//8192*4096
#endif
unsigned char m_imgbuf[56604800];//10640*5320 
unsigned char m_imgbufNew[56604800];//10640*5320 
BITMAPINFO *BI;

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;
METHODDEF(void)
	my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

static BITMAPINFO* BmpInfo()
{
	static BYTE buf[sizeof(BITMAPINFOHEADER)+256*sizeof(RGBQUAD)];
	static BITMAPINFO* info = 0;
	if(info!=0)
		return info;
	info = (BITMAPINFO*)buf;
	memset(buf,0,sizeof(buf));
	BITMAPINFOHEADER& H = info->bmiHeader;
	H.biSize = 40;
	H.biPlanes = 1;
	H.biBitCount = 8;
	RGBQUAD* Q = info->bmiColors;
	for(int i=256; --i>=0;)
	{
		Q[i].rgbBlue = Q[i].rgbGreen = Q[i].rgbRed = i;
		Q[i].rgbReserved = 0;
	}
	return info;
}
//单通道输出   测试
int write_JPEG_file(string strImageName, unsigned char* imgbuffer, int quality, int imgwidth, int imgheight)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	cinfo.image_width = imgwidth;
	cinfo.image_height = imgheight;
	cinfo.input_components = 1;
	cinfo.in_color_space = JCS_GRAYSCALE;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	if ((outfile = fopen(strImageName.c_str(), "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", strImageName);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &imgbuffer[cinfo.next_scanline * cinfo.image_width];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);

	return 0;
}

int write_JPEG_file3(string strImageName, unsigned char* imgbuffer, int quality, int imgwidth, int imgheight)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * outfile;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	cinfo.image_width = imgwidth;
	cinfo.image_height = imgheight;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	if ((outfile = fopen(strImageName.c_str(), "wb")) == NULL)
	{
		fprintf(stderr, "can't open %s\n", strImageName);
		return -1;
	}
	jpeg_stdio_dest(&cinfo, outfile);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &imgbuffer[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);

	return 0;
}


bool read_JPEG_file(const char* strImageName, unsigned char* image_buffer, int* destimgwidth, int* destimgheight)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];

	//memset(m_imgbuf, 0, 33554432);
	memset(m_imgbuf, 0, 56604800);

	FILE *infile = fopen(strImageName, "rb");
	//if (infile == NULL) 
	//{
	//	fprintf(stderr, "can't open %s\n", strImageName);
	//	return false;
	//}
	//fclose(infile);

	//infile = fopen(strImageName, "rb");
	if (infile == NULL) 
	{
		fprintf(stderr, "can't open %s\n", strImageName);
		return false;
	}

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) 
	{
		jpeg_destroy_decompress(&cinfo);
		
		errno_t t =	fclose(infile);
		return 0;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void) jpeg_read_header(&cinfo, TRUE);
	
	*destimgwidth = cinfo.image_width;
	*destimgheight = cinfo.image_height;
	int imgwidth = ((cinfo.image_width + 3) / 4) * 4;

	jpeg_start_decompress(&cinfo);
	picInfo = cinfo.output_components;
	if(cinfo.output_components == 1)
	{		
		while (cinfo.output_scanline < cinfo.output_height) 
		{
			//row_pointer[0] = &image_buffer[cinfo.output_scanline * cinfo.output_width];
			row_pointer[0] = &image_buffer[cinfo.output_scanline * imgwidth];
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
		}
	}
	else if(cinfo.output_components == 3)
	{
		unsigned char* tbuffer = new unsigned char[cinfo.output_width * cinfo.output_components];

		while (cinfo.output_scanline < cinfo.output_height) 
		{
			row_pointer[0] = &tbuffer[0];
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
			//cwb
			for (int i = 0; i < cinfo.output_width*cinfo.output_components-2; i+=2)
			{
				image_buffer[(cinfo.output_scanline - 1) *cinfo.output_width* cinfo.output_components + i] = tbuffer[i];
				image_buffer[(cinfo.output_scanline - 1) *cinfo.output_width* cinfo.output_components + i+1] = tbuffer[i+1];
				image_buffer[(cinfo.output_scanline - 1) *cinfo.output_width* cinfo.output_components + i+2] = tbuffer[i+2];
			}

		}
	}
//	write_JPEG_file3("D:\\tst\\test.jpg", image_buffer, 85, cinfo.output_width, cinfo.output_height);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);



	return true;
}


/*
* 2023年9月11日陈智超注释
* 获取图片的二进制流，原始图片的流，未经过处理的
*/
extern "C" __declspec(dllexport) void
YG_GetImgBuf(BYTE* destbits)
{
	//memcpy(destbits, m_imgbuf, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight);
	//cwb
	memcpy(destbits, m_imgbuf, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight*picInfo);
	//write_JPEG_file3("D:\\tst\\testO.jpg", destbits, 85, BI->bmiHeader.biWidth, BI->bmiHeader.biHeight);
	//write_JPEG_file("D:\\tst\\testO单.jpg", m_imgbuf, 85, BI->bmiHeader.biWidth, BI->bmiHeader.biHeight);
}

void init_bm()
{
	int i;
	static int ii = 0;
	if(ii!=0) return;
	ii = 1;

	for(i=256; --i>=0;)
		m_map1[i] = i;
	for(i=8192; --i>=0;)
	{  
		memcpy(m_mm+i*256,m_map1,256);
		m_map2[i] = m_mm+i*256;
	}

	memset(m_imgbuf, 0, 33554432);

	BI = BmpInfo();
}

static inline double c2c_base(double zm,int c)
{
	double c0;
	c0 = (1+zm)*128;
	if(c0>128) return c;
	return c*130/(c0+10);
}

//描述：根据亮度、对比度、照明参数算出灰度映射关系
static void C2C_Gen(int ld,int dbd,double zm,BYTE* c2c)
{
	double k,b; int i,c;
	k = 1+dbd*0.1;
	b = (ld-5)*10;
	
	for(i=256; --i>=0;)
	{
		c = (int)(c2c_base(zm,i)*k+b);
		if(c>=256) c=255;
		if(c<0) c = 0;
		c2c[i] = c;
	}
}

extern "C" __declspec(dllexport) void
YG_SetPara(int ld,int dbd, int width)
{
	m_ld = ld; m_dbd = dbd;
	C2C_Gen(ld,dbd,0,m_map1);
	for(int i=width; --i>=0;)
	{
		C2C_Gen(ld,dbd,m_zm[i],m_mm+i*256);
	}
}

extern "C" __declspec(dllexport) void
YG_GetPara(int* ld,int* dbd)
{
	*ld = m_ld; *dbd = m_dbd;
}

extern "C" __declspec(dllexport) int
YG_Mode(int mode)
{
	int n = m_mode;
	m_mode = mode;
	return n;
}

static void _map(int n,BYTE* M,BYTE* B)
{
	if(M==0 || B==0)
		return;
	for(;--n>=0; ++B)
	{
		*B = M[*B];
	}
}

static void _map2(int n,BYTE* M,BYTE* B,int w)
{
	if(M==0 || B==0)
		return;
	for(;--n>=0; B+=w)
	{
		*B = M[*B];
	}
}

extern "C" __declspec(dllexport) void 
YG_Adjust(int w,int h,BYTE* bits)
{
	if((m_mode&1)==0)
		return;
	if(m_mode==1)
	{
		_map(w*h,m_map1,bits);
		return;
	}
	if(m_mode==3)
	{
		for(int i=w; --i>=0; )
		{
			_map2(h,m_map2[i],bits+i,w);
		}
		return;
	}
}

extern "C" __declspec(dllexport) void 
	YG_Adjust2()
{
	if((m_mode&1)==0)
		return;
	if(m_mode==1)
	{
		_map(BI->bmiHeader.biWidth*BI->bmiHeader.biHeight*picInfo,m_map1,m_imgbufNew);
		return;
	}
	if(m_mode==3)
	{
		for(int i=BI->bmiHeader.biWidth*picInfo; --i>=0; )
		{
			_map2(BI->bmiHeader.biHeight,m_map2[i], m_imgbufNew+i, BI->bmiHeader.biWidth*picInfo);
		}
		return;
	}
}

extern "C" __declspec(dllexport) void 
	YG_Copy(int w,int h,BYTE* srcbits,BYTE* destbits)
{
	memcpy(destbits, srcbits, w*h);
}

extern "C" __declspec(dllexport) void 
	YG_Recover()
{
	//memcpy(m_imgbufNew, m_imgbuf, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight);
	//cwb
	memcpy(m_imgbufNew, m_imgbuf, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight*picInfo);

}



//描述：获取照明分析参数
//参数：照明分析参数
extern "C" __declspec(dllexport) 
	void YG_ZmGet(float* zm, int width)
{
	memcpy(zm, m_zm, width * sizeof(float));
}

/*
* 2023年9月11日陈智超注释
* 获取图片的二进制流，经过处理的图片流
*/
extern "C" __declspec(dllexport) 
	void YG_GetImgBufNew(BYTE* destbits)
{

	//memcpy(destbits, m_imgbufNew, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight);
	//cwb
	memcpy(destbits, m_imgbufNew, BI->bmiHeader.biWidth * BI->bmiHeader.biHeight *picInfo);

	
//	write_JPEG_file3("D:\\tst\\test1.jpg", m_imgbufNew, 85, BI->bmiHeader.biWidth, BI->bmiHeader.biHeight);
}

//描述：设置照明分析参数
//参数：照明分析参数
extern "C" __declspec(dllexport) 
	void YG_ZmSet(float* zm, int width)
{
	memcpy(m_zm, zm, width * sizeof(float));
}

//描述：加这个函数防止360判本dll有毒
extern "C" __declspec(dllexport) 
	void Fuck360(BYTE* B)
{
}

////描述：从图像获取照明分析参数
////参数：图像的首地址
//extern "C" __declspec(dllexport) 
//	void YG_ZmGen(BYTE* B, int width, int height)
//{
//	if(B==NULL) return;
//
//    int i; 
//	double v;
//    memset(m_tmpSumA, 0, width*sizeof(int));
//
//    for(i=width*height; --i>=0;)
//    {
//        m_tmpSumA[i%width] += B[i];
//    }
//    for(i=width; --i>=0;)
//    {
//        v = (double)(m_tmpSumA[i])/height;
//        v = (v-128)/128;
//        m_zm[i] = (float)v;
//    }
//}

//描述：从图像获取照明分析参数
//参数：图像的首地址
extern "C" __declspec(dllexport) 
	void YG_ZmGen()
{
	if(m_imgbuf==NULL) return;

	int i; 
	double v;
	memset(m_tmpSumA, 0, BI->bmiHeader.biWidth*sizeof(int));

	for(i=BI->bmiHeader.biWidth*BI->bmiHeader.biHeight; --i>=0;)
	{
		m_tmpSumA[i%BI->bmiHeader.biWidth] += m_imgbuf[i];
	}
	for(i=BI->bmiHeader.biWidth; --i>=0;)
	{
		v = (double)(m_tmpSumA[i])/BI->bmiHeader.biHeight;
		v = (v-128)/128;
		m_zm[i] = (float)v;
	}
}

/*
 * 2023年9月11日：陈智超注释：
 * 接口描述：加载图片，传入图片的路径 和图片的宽高
 */
extern "C" __declspec(dllexport) int YG_LoadImg(const char* fpath, int* imgw, int* imgh)
{
	read_JPEG_file(fpath, m_imgbuf, imgw, imgh);

	BI->bmiHeader.biWidth = ((*imgw+3)/4)*4;
	BI->bmiHeader.biHeight = *imgh;
	return picInfo;
}

extern "C" __declspec(dllexport) void YG_PaintImg(HWND hmain)
{
	if(hmain==NULL) return;

	RECT rc; 
	::GetClientRect(hmain,&rc);

	HDC hdc = GetDC(hmain);

	SetStretchBltMode(hdc,STRETCH_HALFTONE);
	StretchDIBits(hdc, 0, 0, rc.right, rc.bottom, 
		0, BI->bmiHeader.biHeight, BI->bmiHeader.biWidth, -BI->bmiHeader.biHeight, 
		m_imgbuf, BI, DIB_RGB_COLORS, SRCCOPY);

	ReleaseDC(hmain,hdc);
}
/////////////////////// 以下是图像蜕化的代码 /////////////////////// 

#include <algorithm>
#include <numeric>

int m_rh=0;  //是否支持锐化
int m_rh_r=0;   //锐化半径
int m_rh_d=0;   //锐化度

extern "C" __declspec(dllexport) int
YG_RH_Set(int rh,int* pr,int* pd)
{
	int r,d;
	r = m_rh; m_rh=rh; rh=r;
	r = m_rh_r; d = m_rh_d;
	m_rh_r = *pr; m_rh_d = *pd;
	*pr = r; *pd = d;
	return rh;
}

static void GetColumn(int w,int h,BYTE* bits,int x,BYTE* p)
{
	BYTE* q = bits+x;
	for(int i=h; --i>=0; )
	{
		*p++ = *q;
		q += w;
	}
}

static void SetColumn(int w,int h,BYTE* bits,int x,BYTE* p)
{
	BYTE* q = bits+x;
	for(int i=h; --i>=0; )
	{
		*q = *p++;
		q += w;
	}
}

static void Filter_Lowpass(int n,const BYTE* P,int radius,BYTE* Q)
{
	int i,m,sum;
	if(n<=2*radius)
	{
		sum = std::accumulate(P,P+n,0)/n;
		std::fill_n(Q,n,(BYTE)sum);
		return;
	}
	sum = std::accumulate(P,P+radius,0);
	m = radius;
	for(i=0; i<n; ++i)
	{
		if(i+radius<n)
		{
			sum += P[i+radius];
			++m;
		}
		if(i-radius>0)
		{
			sum -= P[i-radius-1];
			--m;
		}
		Q[i] = (BYTE)(sum/m);
	}
}

static BYTE* Rh_Alloc(int cs)
{
	static BYTE* _mem = 0;
	static int _cs=0;
	if(cs<=_cs)
		return _mem;
	if(_mem!=NULL)
		delete _mem;
	_mem = new BYTE[_cs=cs];
	return _mem;
}

void ruihua1(int w,int r,int c,BYTE* P)
{
	int x,y,d;
	for(y=r; --y>=0; P+=w)
	{
		for(x=0; x<r; ++x)
		{
			d = P[x]-c;
			d = d*(4+m_rh_d)/4;
			d = c+d;
			if(d<0) d=0;
			if(d>255) d=255;
			P[x] = d;
		}
	}
}

void ruihua(int w,int r,BYTE* bits)
{
	int i,c;
	memset(m_tmpSumA,0,(w/r)*sizeof(int));
	for(i=w*r; --i>=0;)
	{
		m_tmpSumA[(i%w)/r] += bits[i];
	}
	for(i=(w/r); --i>=0;)
	{
		c = m_tmpSumA[i]/r/r;
		ruihua1(w,r,c,bits+i*r);
	}
}

//功能: 实现图片的蜕化效果
extern "C" __declspec(dllexport) void
YG_Ruihua(int w,int h,BYTE* bits)
{
	if(!m_rh || m_rh_r<=0 || m_rh_d<=0)
		return;
	int y,r;
	r = 1<<m_rh_r;
	for(y=0; y+r<=h; y+=r)
	{
		ruihua(w,r,bits+y*w);
	}
}

//功能: 实现图片的蜕化效果
extern "C" __declspec(dllexport) void YG_Ruihua2()
{
	if(!m_rh || m_rh_r<=0 || m_rh_d<=0)
		return;
	int y,r;
	r = 1<<m_rh_r;
	for(y=0; y+r<=BI->bmiHeader.biHeight; y+=r)
	{
		ruihua(BI->bmiHeader.biWidth*picInfo,r,m_imgbufNew+y*BI->bmiHeader.biWidth*picInfo);
	}
}