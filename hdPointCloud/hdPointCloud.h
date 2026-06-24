///*! PointCloud.h
//********************************************************************************
//<PRE>
//模块名       : hdCommon
//文件名       : PointCloud.h
//相关文件     : PointCloud.cpp
//文件实现功能 : <类实现功能.如果包含多个类或结构体,注明相关类或结构体>
//版本         : 1.0
//版权		 : CopyRight @ 2013 海达数云
//--------------------------------------------------------------------------------
//备注         : <其它说明>
//--------------------------------------------------------------------------------
//修改记录 : 
//日 期        版本     修改人              修改内容
//2012/05/31   1.0      <作者>			  创建    
//</PRE>
//*******************************************************************************/
//
//namespace hd
//{
//	namespace scene
//	{
//
//	}
//}
//
//// 禁止使用拷贝极造函数和赋值操作 
//class Foo { 
//public: 
//	explicit Foo(int f); 
//	~Foo(); 
//
//private: 
//	Foo(const Foo& other);
//	void operator=(const Foo&);
//}; 
//
//class DBConn
//{
//
//};
//
//DBConn::~DBConn()
//{
//	try
//	{
//		db.close();
//	}
//	catch (...)
//	{		
//		//std::abort();	// 吞掉异常,或抢先终止
//	}
//}
//
//class PointXYZIPRGBA
//{
//	...
//};
//
//void Fun(std::vector<PointXYZIPRGBA> pts);			// 错误的低效做法
//
//void Fun(const std::vector<PointXYZIPRGBA>& pts);	// 正确的做法

// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 HDPOINTCLOUD_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// HDPOINTCLOUD_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef HDPOINTCLOUD_EXPORTS
#define HDPOINTCLOUD_API __declspec(dllexport)
#else
#define HDPOINTCLOUD_API __declspec(dllimport)
#endif

typedef void (*ProcessCallbackFunc)(float,const char*);