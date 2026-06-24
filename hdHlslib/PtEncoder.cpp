//注意此处的宏定义需要写在#include "PointEncoderDll.h"之前  
//以完成在dll项目内部使用__declspec(dllexport)导出  
//在dll项目外部使用时，用__declspec(dllimport)导入 

#define DLL_IMPLEMENT

#include "PtEncoder.h"
#include "..\hdCommon\point_types2.h"
#include <stdio.h>
#include <string.h>

/*********************************************************
*********         类 CPtXYZEncoder               *********
*********************************************************/
CPtXYZEncoder::CPtXYZEncoder()
{
	m_point_num = 0;
	m_pPoints = NULL;
	m_buffer_size = 0;
	m_userBuffer = NULL;
}
CPtXYZEncoder::CPtXYZEncoder(HdPointXYZ *points, int num)
{
	m_pPoints = points;
	m_point_num = num;
	m_buffer_size = 0;
	m_userBuffer = NULL;
	//计算空间
	int size = sizeof(HdPointXYZ)*m_point_num*1.5;
	m_buffer_size_max =  (size> FASTAC_MAX_SIZE)? FASTAC_MAX_SIZE:size;
	m_buffer_size_max = (m_buffer_size_max>FASTAC_MIN_SIZE)? m_buffer_size_max : FASTAC_MIN_SIZE;
}
CPtXYZEncoder::CPtXYZEncoder(unsigned char* codedBuf, int bufSize, int pointNum)
{
	setCodedBuf(codedBuf, bufSize, pointNum);
}
CPtXYZEncoder::~CPtXYZEncoder()
{
	clear();
}
void CPtXYZEncoder::encodePoints()
{
	if (m_point_num <= 0 || m_pPoints == NULL)    //数据为空
	{
		return ;
	}

	Arithmetic_Codec acoder(m_buffer_size_max);
	Adaptive_Data_Model point_model(256);								// 点模型
	Adaptive_Data_Model bits_model_x(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model bits_model_y(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model bits_model_z(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model* point_model_x_h = new Adaptive_Data_Model[8];	// x坐标高8位模型
	Adaptive_Data_Model* point_model_x_l = new Adaptive_Data_Model[8];	// x坐标低8位模型
	Adaptive_Data_Model* point_model_y_h = new Adaptive_Data_Model[8];	// y坐标高8位模型
	Adaptive_Data_Model* point_model_y_l = new Adaptive_Data_Model[8];	// y坐标低8位模型
	Adaptive_Data_Model* point_model_z_h = new Adaptive_Data_Model[8];	// z坐标高8位模型
	Adaptive_Data_Model* point_model_z_l = new Adaptive_Data_Model[8];	// z坐标低8位模型
	for (int i=0; i<8; i++)
	{
		point_model_x_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_x_l[i].set_alphabet(1<<(i+1));	//(256);
		point_model_y_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_y_l[i].set_alphabet(1<<(i+1));	//(256);
		point_model_z_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_z_l[i].set_alphabet(1<<(i+1));	//(256);
	}

	acoder.start_encoder();   //开始编码
	//第一个点
	unsigned char* data_bytes;
	data_bytes = (unsigned char *) &m_pPoints[0];
	int size = sizeof(HdPointXYZ);
	for (int n = 0; n<size; n++)
	{
		acoder.encode(data_bytes[n], point_model);
	}

	for (int i = 1; i<m_point_num; i++)
	{
		I16 x = m_pPoints[i].x - m_pPoints[i-1].x;
		I16 y = m_pPoints[i].y - m_pPoints[i-1].y;
		I16 z = m_pPoints[i].z - m_pPoints[i-1].z;

		encodeData(x, acoder, bits_model_x, point_model_x_l, point_model_x_h);
		encodeData(y, acoder, bits_model_y, point_model_y_l, point_model_y_h);
		encodeData(z, acoder, bits_model_z, point_model_z_l, point_model_z_h);
		
		
	}
	//	delete data_bytes;
	m_buffer_size = acoder.stop_encoder();
	m_userBuffer = new unsigned char[m_buffer_size];
	memcpy(m_userBuffer, acoder.buffer(),m_buffer_size);

	delete[] point_model_x_h;
	delete[] point_model_x_l;
	delete[] point_model_y_h;
	delete[] point_model_y_l;
	delete[] point_model_z_h;
	delete[] point_model_z_l;
}

void CPtXYZEncoder::encodeData(I16 data, Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l, Adaptive_Data_Model* &model_h)
{
	unsigned short data_fabs;    // 数据的绝对值
	unsigned int k = getHigh(data);
	acoder.encode(k, bits_model);
	if (k > 0)
	{
		// bits = 0, 则x = 0，无需压缩；bits > 0,则x != 0
		// 将x_fabs的值归化到[0，2^k-1]
		if (data < 0)  // x = [-(2^k -1), -2^(k-1)]
		{
			// x 归化到[0， 2^(k-1) - 1]
			data_fabs = data + (1<<k) - 1;   // x = x + 2^(k) -1
		}
		if (data > 0)  // x = [2^(k-1), 2^k -1]
		{
			// x归化到 [2^(k-1), 2^k -1]
			data_fabs = data;
		}
		if (k <= 8) 
		{
			// 有效值不超过8位,直接压缩
			acoder.encode(data_fabs, model_l[k-1]);
		}
		else 
		{
			// 8<k<=16,分高低位压缩
			// 高位数据及位数
			unsigned char k_h = k - 8;
			unsigned char data_h = data_fabs >> 8;
			// 低位数据及位数
			unsigned char k_l = 8;
			unsigned char data_l = data_fabs^(data_h<<8);
			// 分别压缩高八位和低八位数据
			acoder.encode(data_h, model_h[k_h-1]);
			acoder.encode(data_l, model_l[k_l-1]);
		}
	}
}

void CPtXYZEncoder::decodePoints(HdPointXYZ* &points)
{
	if (!points || m_point_num<=0)
	{
//		puts("no data!");
		return;
	}
//	puts("start decode");
	Arithmetic_Codec acoder(m_buffer_size, m_userBuffer);
	Adaptive_Data_Model point_model(256);								// 点模型
	Adaptive_Data_Model bits_model_x(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model bits_model_y(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model bits_model_z(17);							        // 数据的最高有效位数模型，取值0-16
	Adaptive_Data_Model* point_model_x_h = new Adaptive_Data_Model[8];	// x坐标高8位模型
	Adaptive_Data_Model* point_model_x_l = new Adaptive_Data_Model[8];	// x坐标低8位模型
	Adaptive_Data_Model* point_model_y_h = new Adaptive_Data_Model[8];	// y坐标高8位模型
	Adaptive_Data_Model* point_model_y_l = new Adaptive_Data_Model[8];	// y坐标低8位模型
	Adaptive_Data_Model* point_model_z_h = new Adaptive_Data_Model[8];	// z坐标高8位模型
	Adaptive_Data_Model* point_model_z_l = new Adaptive_Data_Model[8];	// z坐标低8位模型
	for (int i=0; i<8; i++)
	{
		point_model_x_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_x_l[i].set_alphabet(1<<(i+1));	//(256);
		point_model_y_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_y_l[i].set_alphabet(1<<(i+1));	//(256);
		point_model_z_h[i].set_alphabet(1<<(i+1));	//(256);
		point_model_z_l[i].set_alphabet(1<<(i+1));	//(256);
	}

	acoder.start_decoder();  //开始解码

	int size = sizeof(HdPointXYZ);    //每个坐标的大小
	unsigned char * ptBuf = new unsigned char[size];
	//第一个点
	for (int i = 0; i<size; i++)
	{
		*(ptBuf+i) = acoder.decode(point_model);
	}
	points[0] = *(HdPointXYZ *)ptBuf;

	for (int i=1;i<m_point_num;i++)
	{
		I16 x, y, z;
		x = decodeData(acoder, bits_model_x, point_model_x_l, point_model_x_h);
		y = decodeData(acoder, bits_model_y, point_model_y_l, point_model_y_h);
		z = decodeData(acoder, bits_model_z, point_model_z_l, point_model_z_h);

		points[i].x = x + points[i-1].x;
		points[i].y = y + points[i-1].y;
		points[i].z = z + points[i-1].z;
	}
	acoder.stop_decoder();
//	delete[] cx;
	delete[] ptBuf;
	delete[] point_model_x_h;
	delete[] point_model_x_l;
	delete[] point_model_y_h;
	delete[] point_model_y_l;
	delete[] point_model_z_h;
	delete[] point_model_z_l;
	return;
}

I16 CPtXYZEncoder::decodeData(Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l, Adaptive_Data_Model* &model_h)
{
	I16 result = 0;
	unsigned short x_fabs;
	unsigned char k = acoder.decode(bits_model);
	if (k)
	{

		if (k<=8)
		{
			x_fabs = acoder.decode(model_l[k-1]);
		}
		else
		{
			unsigned char k_h = k - 8;
			unsigned char x_h = acoder.decode(model_h[k_h-1]);
			unsigned char k_l = 8;
			unsigned char x_l = acoder.decode(model_l[k_l-1]);
			x_fabs = (x_h<<8) | x_l;
		}
		if (x_fabs < (1<<(k-1)))
		{
			result = x_fabs - (1<<k) + 1;
		}
		else
		{
			result = x_fabs;
		}

	}
	else
	{
		result = 0;
	}
	return result;
}

int CPtXYZEncoder::getPointNum()
{
	return m_point_num;
}

unsigned char* CPtXYZEncoder::getBuffer()
{
	return m_userBuffer;
}
int CPtXYZEncoder::getBufferSize()
{
	return m_buffer_size;
}

void CPtXYZEncoder::clear()
{
	if (m_userBuffer)
	{
		delete[] m_userBuffer;
		m_userBuffer = NULL;
	}
	m_pPoints = NULL;
	m_point_num = 0;
	m_buffer_size = 0;

}

void CPtXYZEncoder::setPoints(HdPointXYZ *points)
{
	m_pPoints = points;
}

void CPtXYZEncoder::setPointNum(int pointNum)
{
	m_point_num = pointNum;
	//计算空间
	int size = sizeof(HdPointXYZ)*m_point_num*1.5;
	m_buffer_size_max =  (size> FASTAC_MAX_SIZE)? FASTAC_MAX_SIZE:size;
	m_buffer_size_max = (m_buffer_size_max>FASTAC_MIN_SIZE)? m_buffer_size_max : FASTAC_MIN_SIZE;
}

void CPtXYZEncoder::setCodedBuf(unsigned char* buf, int size, int pointNum)
{
	m_buffer_size = (size>FASTAC_MIN_SIZE) ? size:FASTAC_MIN_SIZE;    //FASTAC允许的最小内存处理单元为16bytes
	m_point_num = pointNum;
	m_userBuffer = new unsigned char[m_buffer_size];
	memcpy(m_userBuffer, buf, m_buffer_size);
}

unsigned char CPtXYZEncoder::getHigh(int data)
{
	unsigned char k = 0;
	// 求data的绝对值， 调整data，
	unsigned int c = data <= 0 ? -data : data; //(data-1); 
	while (c)
	{
		c = c>>1;
		k++;
	}
	return k;
}

/*********************************************************
 *********         类 CPtIEncoder               *********
*********************************************************/
// 默认构造函数
CPtIEncoder::CPtIEncoder()
	:m_num(0),m_intensity(NULL),m_buffer_size(0),m_user_buffer(NULL)
{
}

// 析构函数
CPtIEncoder::~CPtIEncoder()
{
	clear();
}

// 压缩数据，使用的构造函数
CPtIEncoder::CPtIEncoder(HdIntensity *intensity, int num)
	:m_num(num),m_intensity(intensity),m_buffer_size(0),m_user_buffer(NULL)
{
	// 计算最大空间量
	int size = m_num * sizeof(HdIntensity) * 1.5;
	m_buffer_size_max =  (size > FASTAC_MAX_SIZE) ? FASTAC_MAX_SIZE : size;
	m_buffer_size_max = (m_buffer_size_max > FASTAC_MIN_SIZE) ? m_buffer_size_max : FASTAC_MIN_SIZE;
}

//参数： codeedBuf 点压缩后的数据流； bufSize 数据的大小； num 实际上点的个数
//作用： 解压数据，使用的构造函数
CPtIEncoder::CPtIEncoder(unsigned char* coded_buffer, int buffer_size, int num)
	:m_num(num),m_intensity(NULL),m_buffer_size(0),m_user_buffer(NULL)
{
	setCodedBuf(coded_buffer, buffer_size, num);
}

// 输入需要解压的数据
void CPtIEncoder::setCodedBuf(unsigned char* buffer, int size, int num)
{
	//FASTAC允许的最小内存处理单元为16bytes
	m_buffer_size = (size > FASTAC_MIN_SIZE) ? size : FASTAC_MIN_SIZE;    
	m_num = num;
	if (m_user_buffer)
	{
		delete[] m_user_buffer;
		m_user_buffer = NULL;
	}
	m_user_buffer = new unsigned char[m_buffer_size];
	memcpy(m_user_buffer, buffer, m_buffer_size);
}

// 压缩数据
void CPtIEncoder::encodeInten()
{
	//数据为空
	if (m_num <= 0 || m_intensity == NULL)   
	{
		return ;
	}

	Adaptive_Data_Model model(256);
	Adaptive_Data_Model bits_model(9);

	Adaptive_Data_Model* model_i = new Adaptive_Data_Model[8];

	for (int i=0; i<8; i++)
	{
		model_i[i].set_alphabet(1<<(i+1));	//(256);
	}

	Arithmetic_Codec acoder(m_buffer_size_max);

	//开始编码
	acoder.start_encoder();  

	//第一个点
	unsigned char* data_bytes;
	data_bytes = (unsigned char *) &m_intensity[0];
	acoder.encode(data_bytes[0], model);

	for (int i = 1; i<m_num; i++)
	{
		s8 inten = m_intensity[i] - m_intensity[i-1];

		encodeData(inten, acoder, bits_model, model_i);
	}

	//	delete data_bytes;
	m_buffer_size = acoder.stop_encoder();
	m_user_buffer = new unsigned char[m_buffer_size];
	memcpy(m_user_buffer, acoder.buffer(),m_buffer_size);

	delete[] model_i;
	model_i = NULL;
}

// 解压数据
// 输出： points 解压后的数据, 需要事先分配好内存
void CPtIEncoder::decodeInten(HdIntensity* &intensity)
{
	if (!intensity || m_num<=0)
	{
		return;
	}

	Adaptive_Data_Model model(256);
	Adaptive_Data_Model bits_model(9);

	Adaptive_Data_Model* model_i = new Adaptive_Data_Model[8];

	for (int i=0; i<8; i++)
	{
		model_i[i].set_alphabet(1<<(i+1));	//(256);
	}

	//	puts("start decode");
	Arithmetic_Codec acoder(m_buffer_size, m_user_buffer);

	//开始解码
	acoder.start_decoder(); 

	//每个坐标的大小
	int size = sizeof(HdIntensity);    
	unsigned char temp_buffer;

	//第一个点
	temp_buffer = acoder.decode(model);

	intensity[0] = temp_buffer;

	for (int i = 1; i < m_num; i++)
	{
		s8 inten = decodeData(acoder, bits_model, model_i);

		intensity[i] = inten + intensity[i-1];
	}

	acoder.stop_decoder();
	delete[] model_i;
	model_i = NULL;
	return;
}

// 获取点的数量（数组长度）
int CPtIEncoder::getNum()
{
	return m_num;
}

// 获取压缩后的数据的首地址
unsigned char* CPtIEncoder::getBuffer()
{
	return m_user_buffer;
}

// 获取压缩后的文件大小
int CPtIEncoder::getBufferSize()
{
	return m_buffer_size;
}

// 清空
void CPtIEncoder::clear()
{
	if (m_user_buffer)
	{
		delete[] m_user_buffer;
		m_user_buffer = NULL;
	}

	m_intensity = NULL;
	m_num = 0;
	m_buffer_size = 0;

}

// 求最高位
u8 CPtIEncoder::getHigh(s16 data)
{
	unsigned char k = 0;

	// 求data的绝对值， 调整data，
	unsigned int c = data <= 0 ? -data : data; //(data-1); 
	while (c)
	{
		c = c>>1;
		k++;
	}

	return k;
}

// 压缩数据, 压缩整型数据
void CPtIEncoder::encodeData(s16 data, Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model)
{
	unsigned short data_fabs;    // 数据的绝对值
	unsigned int k = getHigh(data);
	acoder.encode(k, bits_model);

	if (k > 0)
	{
		// bits = 0, 则x = 0，无需压缩；bits > 0,则x != 0
		// 将x_fabs的值归化到[0，2^k-1]
		if (data < 0)  // x = [-(2^k -1), -2^(k-1)]
		{
			// x 归化到[0， 2^(k-1) - 1]
			data_fabs = data + (1<<k) - 1;   // x = x + 2^(k) -1
		}

		if (data > 0)  // x = [2^(k-1), 2^k -1]
		{
			// x归化到 [2^(k-1), 2^k -1]
			data_fabs = data;
		}
		acoder.encode(data_fabs, model[k-1]);
	}
}

// 解压, 压缩整型数据
s8 CPtIEncoder::decodeData(Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l)
{
	s8 result = 0;
	u8 fabs;
	u8 k = acoder.decode(bits_model);
	if (k)
	{
		fabs = acoder.decode(model_l[k-1]);

		if (fabs < (1<<(k-1)))
		{
			result = fabs - (1<<k) + 1;
		}
		else
		{
			result = fabs;
		}
	}
	else
	{
		result = 0;
	}

	return result;
}