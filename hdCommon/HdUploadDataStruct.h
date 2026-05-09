/*!@HdUploadDataStruct
*******************************************************************************************************
<PRE>
模块名      : iScanProjectUpload
文件名      : HdUploadDataStruct.h
相关文件    : 
文件实现功能: 数据上传功能中使用的数据结构
作者        : 李夏亮
版本        : 1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日期		版本		修改人		修改内容
2015/10/20	1.0         李夏亮          创建
2015/11/02  1.1         张阳        合并HdUploadRecordStruct.h文件
</PRE>
******************************************************************************************************/
#pragma once
#include "stdafx.h"
#include "../hdCore/hdTime.h"
#include <string>
#include <vector>

using namespace std;

namespace hd
{
	struct hdDatabaseSetting
	{
		string m_strServerName;            //服务器名称
		string m_strPortNum;               //端口号
		string m_strDatabaseName;          //数据库名
		string m_strUserName;              //用户名
		string m_strPassword;              //密码

		hdDatabaseSetting()
		{
			clear();
		}

		void clear()
		{
			m_strServerName = "";
			m_strPortNum = "";
			m_strDatabaseName = "";
			m_strUserName = "";
			m_strPassword = "";
		}
	};

	struct hdCutImageSetting
	{
		double m_dImageQuality;          //图片质量
        int    m_nSelLevel;              //选择切片级别
		int    m_nMaxLevel_max;          //切片最大级别
		bool   m_isHave_X_Level;         //是否有X级别
		int    m_nMaxLevel_X;            //切片级别个数
		int    m_nTileLevel;             //影像总级别

		hdCutImageSetting()
		{
			clear();
		}

		void clear()
		{
			m_dImageQuality = 0.75;
			m_nSelLevel = -1;
			m_nMaxLevel_max = -1;          //切片最大级别
			m_isHave_X_Level = false;     //选择切片级别是否有X级别
			m_nMaxLevel_X = -1;            //切片级别个数
			m_nTileLevel = -1;
		}
	};

    enum THREAD_PROCESSING_STATUS
    {
        THREAD_INITIALISE = -1,         // 未开始
        THREAD_PROCESSING = 0,          // 正在进行中
        THREAD_SUCCESS = 1,             // 处理成功
        THREAD_FAILURE = 2,             // 处理失败
    };

    // 点云HLZ文件断点续传结构体
    struct HlzUploadRecord
    {
        string m_strPrjName;              // 工程名称   (主键, 用于判断是否重复加入点可用于map)
        string m_strFilePath;             // 工程所在本地路径
        int m_nZone;                      // 投影带号
        int m_nLidarID;                   // 扫描头编号
        HDTIME m_tGatherTime;             // 采集时间
        //CHdUploadParam* m_pUploadParam;   // 上传实例

        HWND m_hWnd;                      // 窗体句柄
        int m_index;                      // 索引

        // 记录上传位置
        int m_nLevelID;                   // 层级ID     (-1代表未开始)
        int m_nBlockSetID;                // 块集ID     (-1代表未开始)
        int m_nBlockID;                   // 块ID       (-1代表未开始)
        int m_nParcelID;                  // 包ID       (-1代表未开始)

        // 目标服务器数据
        string m_strSvrName;              // 服务器名称
        string m_strPortNum;              // 端口号
        string m_strDbName;               // 数据库名称

        // 用于多线程标记和断点续传标记
        THREAD_PROCESSING_STATUS m_status;// 处理状态

        HlzUploadRecord()
        {
            m_index = 0;
            m_hWnd = NULL;
            m_nLevelID = 0;
            m_nBlockSetID = 0;
            m_nBlockID = 0;
            m_nParcelID = 0;
            m_status = THREAD_INITIALISE;
            //m_pUploadParam = NULL;
        }

        ~HlzUploadRecord()
        {
            //if (m_pUploadParam)
            //{
            //    delete m_pUploadParam;
            //    m_pUploadParam = NULL;
            //}
        }
    };

    //全景切片断点续传结构体
    struct PanoUploadRecord
    {
        string m_strRouteName;                //轨迹工程的名称
        int m_nEndIndex;                      //上传终止编号
        vector<int> m_vecErrIndex;              //上传失败的索引
        THREAD_PROCESSING_STATUS m_status;    //处理状态
        string m_strSvrName;                  //服务器名称
        string m_strPortNum;                  //端口号
        string m_strDbName;                   //数据库名称

        PanoUploadRecord()
        {
            clear();
        }

        //清理数据
        void clear()
        {
            m_strRouteName = "";
            m_nEndIndex = -1;
            m_vecErrIndex.clear();
            m_status = THREAD_INITIALISE;
            m_strSvrName = "";
            m_strPortNum = "";
            m_strDbName = "";
        }
    };

	//pos信息集合
	struct hdPosInfo
	{
		string m_strPosPath;                  //pos位置信息
		vector<string> m_vecRoutePath;        //route位置信息集合
		HDTIME m_begTime;                     //起始时间
		HDTIME m_endTime;                     //终止时间

		//清理结构体
		void clear()
		{
			m_strPosPath = "";
			m_vecRoutePath.clear();
		}

		hdPosInfo()
		{
			clear();
		}
	};

    // 工程信息上传状态结构体
    struct PrjUploadStatus
    {
        // 目标服务器数据
        string m_strSvrName;              // 服务器名称
        string m_strPortNum;              // 端口号
        string m_strDbName;               // 数据库名称
        string m_strRoutePath;            // 工程名

        // 用于多线程标记和断点续传标记
        THREAD_PROCESSING_STATUS m_status;// 处理状态

        // 构造函数
        PrjUploadStatus()
        {
            m_strSvrName = "";
            m_strPortNum = "";
            m_strDbName = "";
            m_strRoutePath = "";
            m_status = THREAD_INITIALISE;
        }

        // 构造函数
        PrjUploadStatus(string strSvrName, string strPortNum, string strDbName, string strRoutePath)
        {
            m_strSvrName = strSvrName;
            m_strPortNum = strPortNum;
            m_strDbName = strDbName;
            m_strRoutePath = strRoutePath;
            m_status = THREAD_INITIALISE;
        }

        // 析构函数
        ~PrjUploadStatus()
        {
        }
    };

	//数据库中工程信息集合
	struct DbPrjInfo
	{
		string m_strRouteName;           //轨迹工程名
		string m_strImgId;               //轨迹工程中的某个全景名称
		int m_nPanoUpload;              //是否上传全景数据
		vector<int> m_vecNoUploadPcd;   //未上传的点云
		vector<int> m_vecPcdHeader;     //缺失头文件的点云编号

		//初始化数据
		void Clear()
		{
			m_strRouteName = "";
			m_strImgId = "";
			m_nPanoUpload = 1;
			m_vecNoUploadPcd.clear();
			m_vecPcdHeader.clear();
		}

		//构造函数
		DbPrjInfo()
		{
			Clear();
		}

		//析构函数
		~DbPrjInfo()
		{

		}
	};
}
