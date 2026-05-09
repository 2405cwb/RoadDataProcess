/*! HDData.h
********************************************************************************
<PRE>
模块名       : hdCommon
文件名       : HDData.h
相关文件     : HDObject.h HDPoint.h HDLabel.h HDPolyline.h

文件实现功能 : 定义数据对象基类
作者         : 姚立
版本         : 1.0
--------------------------------------------------------------------------------
备注         : 数据对象分为两大类，一类为文件对象，均由CHdFileData类派生，一类为内存对象，
				均由HDObject类派生——HDObject也由HDData派生。
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2012/06/20   1.0      姚立  
2013/03/07   1.1      龚书林		添加全景内存对象
</PRE>
*******************************************************************************/

#pragma once
#include "..\hdCommon.h"
#include "..\HDObject.h"
#include "..\hdParaDTStruct.h"
#include <string>

using namespace std;

namespace hd
{
	
	/************************************************************************/
	/*定义文件类                                                            */
	/************************************************************************/
	class HDCOMMON_API CHdFileData : public CHDObject
	{
	public:
		CHdFileData(const char* strFile)
			:m_strFile(strFile)
		{
		}
		virtual ~CHdFileData() {}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() {return ESDT_FILE;}

	public:
		string m_strFile;		//记录文件路径
	};


	/************************************************************************/
	/*定义点云数据文件类                                                    */
	/************************************************************************/
	class HDCOMMON_API CHdHlsFileData : public CHdFileData
	{
	public:
		CHdHlsFileData(const char* strFile):CHdFileData(strFile)
		{
		}
		~CHdHlsFileData() {}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_FILE_HLS; }
	};


	/************************************************************************/
	/*定义点云灰度图文件类                                                  */
	/************************************************************************/
	class HDCOMMON_API CHdGreyPicFileData : public CHdFileData
	{
	public:
		CHdGreyPicFileData(const char* strFile):CHdFileData(strFile)
		{
			m_fRowStartAngle = 90.0f;
			m_fRowEndAngle = -45.0f;//-62.5f;
			m_fColStartAngle = 0.0f;
			m_fColEndAngle = 360.0f;
		}

		CHdGreyPicFileData(const char* strFile, float fRowStart, float fRowEnd, float fColsStart, float fColEnd)
			:CHdFileData(strFile),m_fRowStartAngle(fRowStart),m_fRowEndAngle(fRowEnd),m_fColStartAngle(fColsStart),m_fColEndAngle(fColEnd)
		{
		}

		~CHdGreyPicFileData() {}

	public:
		float m_fRowStartAngle;		//垂直起始角
		float m_fRowEndAngle;
		float m_fColStartAngle;		//水平起始角
		float m_fColEndAngle;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_FILE_GREY_PIC; }
	};


	/************************************************************************/
	/*定义全景图片文件类                                                    */
	/************************************************************************/
	class HDCOMMON_API CHdPanoPicFileData : public CHdFileData
	{
	public:
		CHdPanoPicFileData(const char* strFile):CHdFileData(strFile)
		{
			m_fRowStartAngle = 90.0;
			m_fRowEndAngle = -90;
			m_fColStartAngle = 0.0;
			m_fColEndAngle = 360.0;
		}

		CHdPanoPicFileData(const char* strFile, double fRowStart, double fRowEnd, double fColsStart, double fColEnd)
			:CHdFileData(strFile),m_fRowStartAngle(fRowStart),m_fRowEndAngle(fRowEnd),m_fColStartAngle(fColsStart),m_fColEndAngle(fColEnd)
		{
		}

		~CHdPanoPicFileData() {}

	public:
		double m_fRowStartAngle;		//垂直起始角
		double m_fRowEndAngle;
		double m_fColStartAngle;		//水平起始角
		double m_fColEndAngle;

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_FILE_PANO_PIC; }
	};
	//! 全景内存流对象
	class HDCOMMON_API CHdPanoData : public CHDObject
	{
	public:
		CHdPanoData(void)
		{
			m_panoData = NULL;
			m_fRowStartAngle = 0.0;
			m_fRowEndAngle = 180;
			m_fColStartAngle = 0.0;
			m_fColEndAngle = 360;
		}
		virtual ~CHdPanoData(void) {};

		// 得到对象的类型
		virtual ENUM_HDMS_OBJECT_TYPE GetType() const{return E_HOT_PANODATA;};

		//! 全景内存数据
		HD_IMAGEDATA* m_panoData;

		double m_fRowStartAngle;		//垂直起始角
		double m_fRowEndAngle;
		double m_fColStartAngle;		//水平起始角
		double m_fColEndAngle;
	};

	/************************************************************************/
	/*定义彩色图片文件类                                                    */
	/************************************************************************/
	class HDCOMMON_API CHdColorPicFileData : public CHdFileData
	{
	public:
		CHdColorPicFileData(const char* strFile):CHdFileData(strFile)
		{
		}

		~CHdColorPicFileData() {}

		virtual ENUM_HDMS_OBJECT_TYPE GetType() const { return ESDT_FILE_COLOR_PIC; }
	};
}
