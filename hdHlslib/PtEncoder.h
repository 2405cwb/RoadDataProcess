/*压缩和解压点的结构体*/

#pragma once

//该宏完成在dll项目内部使用__declspec(dllexport)导出  
//在dll项目外部使用时，用__declspec(dllimport)导入  
//宏DLL_IMPLEMENT在simpledll.cpp中定义  
#ifdef DLL_IMPLEMENT  
#define DLL_API __declspec(dllexport)  
#else  
#define DLL_API __declspec(dllimport)  
#endif

#include "..\hdCommon\point_types2.h"
#include "arithmetic_codec.h"

using namespace hd;


typedef short I16;


#define FASTAC_MAX_SIZE 16777210      //fastac一次性能处理的最大数据量
#define FASTAC_MIN_SIZE 16            //fastac允许的最小内存单元

class DLL_API CPtXYZEncoder
{
public:
	CPtXYZEncoder();

	//参数： points 点数组； num 点的个数
	//作用： 压缩数据，使用的构造函数
	CPtXYZEncoder(HdPointXYZ *points, int num);   

	//参数： codeedBuf 点压缩后的数据流； bufSize 数据的大小； pointNum 实际上点的个数
	//作用： 解压数据，使用的构造函数
	CPtXYZEncoder(unsigned char* codedBuf, int bufSize, int pointNum);  

	//点数组指针
	void setPoints(HdPointXYZ *points);

	//设置点个数
	void setPointNum(int pointNum);

	// 输入需要解压的数据
	void setCodedBuf(unsigned char* buf, int size, int pointNum);

	//压缩数据
	void encodePoints();     

	// 解压数据
	// 输出： points 解压后的数据, 需要事先分配好内存
	void decodePoints(HdPointXYZ* &points);  

	//获取点的数量（数组长度）
	int getPointNum();   

	//获取压缩后的数据的首地址
	unsigned char* getBuffer();    

	//获取压缩后的文件大小
	int getBufferSize();     

	//清空
	void clear();

	//析构函数
	~CPtXYZEncoder();
private:
	HdPointXYZ * m_pPoints;           //点数组
	int m_point_num;                  //点个数
	unsigned char *m_userBuffer;      //压缩后数据内存位置
	int m_buffer_size;                //压缩后占用的空间大小
	int m_buffer_size_max;            //最大空间量

	// 求最高位
	unsigned char getHigh(int data);   
	// 压缩数据, 压缩整型数据
	void encodeData(I16 data, Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l, Adaptive_Data_Model* &model_h);
	// 解压, 压缩整型数据
	I16 decodeData(Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l, Adaptive_Data_Model* &model_h);
};

class DLL_API CPtIEncoder
{
public:
	CPtIEncoder(void);
	~CPtIEncoder(void);

	/*!
	* @brief 构造函数
	* @details 构造函数
	* @param[in] intensity 强度数组
	* @param[in] num		   数组长度
	*/
	CPtIEncoder(HdIntensity *intensity, int num);   

	/*!
	* @brief 构造函数
	* @details 构造函数
	* @param[in] coded_buffer 压缩后数据首地址
	* @param[in] buffer_size 数据的大小
	* @param[in] num           强度数组长度
	*/
	CPtIEncoder(unsigned char* coded_buffer, int buffer_size, int num);  

	/*!
	* @brief 输入需要解压的强度数据
	* @param[in] buffer 点压缩后的数据流
	* @param[in] size 数据的大小
	* @param[in] num 实际强度数组长度
	*/
	void setCodedBuf(unsigned char* buffer, int size, int num);

	//! 压缩数据
	void encodeInten();     

	/*!
	* @brief 解压数据
	* @details 解压数据
	* @param[in] point_buffer 解压后的数据, 需要事先分配好内存
	*/
	void decodeInten(HdIntensity* &intensity);  

	//! 获取强度个数（数组长度）
	int getNum();   

	//! 获取压缩后的数据的首地址
	unsigned char* getBuffer();    

	//! 获取压缩后的文件大小
	int getBufferSize();     

	//! 清空
	void clear();

private:
	
	void encodeData(s16 data, Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model);

	s8 decodeData(Arithmetic_Codec& acoder, Adaptive_Data_Model& bits_model, Adaptive_Data_Model* &model_l);

	u8 getHigh(s16 data);

	//! 强度数组
	HdIntensity * m_intensity; 

	//! 强度数组长度
	int m_num;      

	//! 压缩后数据内存位置
	unsigned char *m_user_buffer;  

	//! 压缩后占用的空间大小
	int m_buffer_size;   

	//! 最大空间量
	int m_buffer_size_max;    

};
