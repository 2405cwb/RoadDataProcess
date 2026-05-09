/*! @hdMongoDBStructDef.h
*******************************************************************************************************
<PRE>
模块名       : hdCommon
文件名       : hdMongoDBStructDef.h
相关文件     : 
文件实现功能 : 定义MongoDB对应的结构体
作者         : 张阳
版本         : 1.0
-------------------------------------------------------
备注：
-------------------------------------------------------
修改记录：
日 期        版本     修改人              修改内容
2015/10/13   1.0      张阳                创建
</PRE>
******************************************************************************************************/

#pragma once
#include "hdHdiStruct.h"
#include "point_types2.h"

namespace hd
{
    // Double 三维数据
    struct HD_3D_DOUBLE_ARRAY
    {
        // 构造函数
        HD_3D_DOUBLE_ARRAY() : dX(0), dY(0), dZ(0)
        {
        }

        // 重载构造函数
        HD_3D_DOUBLE_ARRAY(double dX, double dY, double dZ)
        {
            this->dX = dX;
            this->dY = dY;
            this->dZ = dZ;
        }

        HD_3D_DOUBLE_ARRAY& operator = (const HD_3D_DOUBLE_ARRAY& array)
        {
            if (this != &array)
            {
                dX = array.dX;
                dY = array.dY;
                dZ = array.dZ;
            }

            return *this;
        }

        double dX;                          // 行
        double dY;                          // 列
        double dZ;                          // 高度
    };

    // Double 二维数组
    struct HD_2D_DOUBLE_ARRAY
    {
        // 构造函数
        HD_2D_DOUBLE_ARRAY() : dX(0), dY(0)
        {
        }

        HD_2D_DOUBLE_ARRAY& operator = (const HD_2D_DOUBLE_ARRAY& array)
        {
            if (this != &array)
            {
                dX = array.dX;
                dY = array.dY;
            }

            return *this;
        }

        double dX;                          // 行
        double dY;                          // 列
    };

	// 总体结构表数据类型
	struct HD_ISCANINFO{

		// 构造函数
		HD_ISCANINFO()
		{
			strIScanNo = "";
			strIScanName = "";
			pConfigData = "";
			strIScanPos1 = "";
			strIScanImageTime1 = "";

			for (int i=0; i<3; i++)
			{
				strIScanLin[i] = "";
				strIScanTime[i] = "";
                strIScanPcdHeader[i] = "";
			}

			strIScanPara = "";
			strCarNo = "";
			strMemo = "";
		}

		// 赋值运算符重载
		HD_ISCANINFO& operator = (const HD_ISCANINFO& info)
		{
            if (this != &info)
            {
                strIScanNo = info.strIScanNo;
                strIScanName = info.strIScanName;
                pConfigData = info.pConfigData;
                strIScanPos1 = info.strIScanPos1;
                strIScanImage1 = info.strIScanImage1;

                for (int i=0; i<3; i++)
                {
                    strIScanLin[i] = info.strIScanLin[i];
                    strIScanTime[i] = info.strIScanTime[i];
                    strIScanPcdHeader[i] = info.strIScanPcdHeader[i];
                }

				strIScanPara = info.strIScanPara;
                strCarNo = info.strCarNo;
                strMemo = info.strMemo;
            }

            return *this;
		}

		string	strIScanNo;				// 设备编号
		string	strIScanName;			// 工程名称
		string	pConfigData;			// IScan-Route.config
		string	strIScanPos1;			// POS文件名
		string	strIScanImage1;			// HDI文件名
		string	strIScanImageTime1;		// 相机SYN文件名

		string	strIScanLin[3];			// 扫描仪Lin文件
		string	strIScanTime[3];		// 扫描仪Syn文件
        string strIScanPcdHeader[3];    // 扫描仪Header文件

		string	strIScanPara;			// iScan-Para.db
		string	strCarNo;				// 车牌号
		string	strMemo;				// 备注
	};

	//////////////////////////////////////////ves///////////////////////////////////
	struct HD_QUERYTEST{

		// 构造函数
		HD_QUERYTEST()
		{
			nInt = 0;
			nInt_2 = 0;
			douDouble = 0;
			douDouble_2 = 0;
			strString = "";
			strString_2 = "";
			floFlout = 0;
			floFlout_2 = 0;
			loLong = 0;
			strString_3 = "";
			pBinadata = NULL;
		}

		// 赋值运算符重载
		HD_QUERYTEST& operator = (const HD_QUERYTEST& info)
		{
			if (this != &info)
			{
				nInt = info.nInt;
				nInt_2 = info.nInt_2;
				douDouble = info.douDouble;
				douDouble_2 = info.douDouble_2;
				strString = info.strString;
				strString_2 = info.strString_2;
				floFlout = info.floFlout;
				floFlout_2 = info.floFlout_2;
				loLong = info.loLong;
				strString_3 = info.strString_3;
				pBinadata = info.pBinadata;
			}

			return *this;
		}

		int	    nInt;				
		string	strString;			
		double	douDouble;			
		float	floFlout;			
		long	loLong;			
		string	strString_3;		

		int	    nInt_2;			
		string	strString_2;		
		double  douDouble_2;    
		float	floFlout_2;			
		
		HdPointXYZ* pBinadata;                              
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////

    // 全景轨迹点数据类型
    struct HD_HDI_POINT 
    {
        // 构造函数
        HD_HDI_POINT() : dX(0), dY(0), dZ(0), dB(0), dL(0), dYaw(0), dPitch(0), dRoll(0)
        {
        }

        // 赋值运算符重载
        HD_HDI_POINT& operator = (const HD_HDI_POINT& point)
        {
            // 防止自我复制
            if (&point != this)
            {
                strImageID = point.strImageID;
                strImageName = point.strImageName;
                strRouteID = point.strRouteID;
                nCameraNo = point.nCameraNo;
                tGatherTime = point.tGatherTime;
                dX = point.dX;
                dY = point.dY;
                dZ = point.dZ;
                dB = point.dB;
                dL = point.dL;
                dYaw = point.dYaw;
                dPitch = point.dPitch;
                dRoll = point.dRoll;
                arrayShapeBL = point.arrayShapeBL;
                strPriMemo = point.strPriMemo;
            }

            return *this;
        }

        string strImageID;                  //全景轨迹
        string strImageName;                // 全景站点名称
        string strRouteID;                  // 轨迹ID
        int nCameraNo;                      // 相机号
        HDTIME tGatherTime;                 // 采集时间
        double dX;                          // X坐标
        double dY;                          // Y坐标
        double dZ;                          // Z坐标
        double dB;                          // 纬度
        double dL;                          // 经度
        double dYaw;                        // 相机航向角
        double dPitch;                      // 相机俯仰角
        double dRoll;                       // 相机翻滚角
        HD_3D_DOUBLE_ARRAY arrayShapeBL;    // 轨迹点
        string strPriMemo;                  // 工程备注
    };

    // 全景轨迹线数据类型
    struct HD_HDI_LINE
    {
        // 构造函数
        HD_HDI_LINE() : dXMIN(0), dYMIN(0), dZMIN(0), dBMin(0), dLMin(0)
            , dXMAX(0), dYMAX(0), dZMAX(0), dBMax(0), dLMax(0), nCamera(-1), nPtNum(-1)
        {
        }

        // 重载赋值运算符
        HD_HDI_LINE& operator = (const HD_HDI_LINE& line)
        {
            if (this != &line)
            {
                strHDIID = line.strHDIID;
                strPrjName = line.strPrjName;
                strDeviceNo = line.strDeviceNo;
                nCamera = line.nCamera;
                tStartTime = line.tStartTime;
                tEndTime = line.tEndTime;
                nPtNum = line.nPtNum;
                dXMIN = line.dXMIN;
                dYMIN = line.dYMIN;
                dZMIN = line.dZMIN;
                dBMin = line.dBMin;
                dLMin = line.dLMin;
                dXMAX = line.dXMAX;
                dYMAX = line.dYMAX;
                dZMAX = line.dZMAX;
                dBMax = line.dBMax;
                dLMax = line.dLMax;
                arrayShapePrj = line.arrayShapePrj;
                arrayShapeBL = line.arrayShapeBL;
            }

            return *this;
        }

        string strHDIID;                    // 轨迹标识符
        string strPrjName;                  // 工程名称
        string strDeviceNo;                 // 设备标识
        int nCamera;                        // 相机号
        HDTIME tStartTime;                  // 采集起始时间
        HDTIME tEndTime;                    // 采集结束时间
        int nPtNum;                         // 轨迹点数目
        double dXMIN;                       // X轴坐标最小值
        double dYMIN;                       // Y轴坐标最小值
        double dZMIN;                       // Z轴坐标最小值
        double dXMAX;                       // X轴坐标最大值
        double dYMAX;                       // Y轴坐标最大值
        double dZMAX;                       // Z轴坐标最大值
        double dBMin;                       // 纬度最小值
        double dLMin;                       // 经度最小值
        double dBMax;                       // 纬度最大值
        double dLMax;                       // 经度最大值
        vector<HD_3D_DOUBLE_ARRAY> arrayShapePrj;// 投影坐标几何形状
        vector<HD_3D_DOUBLE_ARRAY> arrayShapeBL;// 经纬度坐标几何形状
    };

	// 点云头文件表数据类型
	struct HD_POINT_CLOUD_HEADER
	{
		// 构造函数
		HD_POINT_CLOUD_HEADER() : dXMIN(0), dYMIN(0), dZMIN(0), dXMAX(0), dYMAX(0), dZMAX(0)
		{
			strIScanName = "";
			nLidarID = -1;
			strHeaderBuf[256] = 0;
			strMemo = "";
		}

		// 重载赋值运算符
		HD_POINT_CLOUD_HEADER& operator = (const HD_POINT_CLOUD_HEADER& PointCloudHeader)
		{
			if (this != &PointCloudHeader)
			{
				strIScanName = PointCloudHeader.strIScanName;
				nLidarID = PointCloudHeader.nLidarID;
				//strcpy(strHeaderBuf, PointCloudHeader.strHeaderBuf);
				memcpy(strHeaderBuf,PointCloudHeader.strHeaderBuf,256);
				dXMIN = PointCloudHeader.dXMIN;
				dYMIN = PointCloudHeader.dYMIN;
				dZMIN = PointCloudHeader.dZMIN;
				dXMAX = PointCloudHeader.dXMAX;
				dYMAX = PointCloudHeader.dYMAX;
				dZMAX = PointCloudHeader.dZMAX;
				strMemo = PointCloudHeader.strMemo;
			}
			return *this;
		}

		string strIScanName;				// 工程名
		int nLidarID;                     // 扫描头编号
		char strHeaderBuf[256];				// 点云头文件数据
		double dXMIN;                       // XMIN, 索引值
		double dYMIN;                       // YMIN, 索引值
		double dZMIN;                       // ZMIN, 索引值
		double dXMAX;                       // XMAX, 索引值
		double dYMAX;                       // YMAX, 索引值
		double dZMAX;                       // ZMAX, 索引值
		string strMemo;						// 备注
	};

    // 点云数据类型
    struct HD_POINT_CLOUD
    {
        // 构造函数
        HD_POINT_CLOUD() : nZone(-1), dXMIN(0), dYMIN(0), dZMIN(0), dXMAX(0), dYMAX(0), dZMAX(0), tPointNum(0), hasData(false)
        {
			strLevelID = -1;
			strParcelID = "";
			nZone = -1;
			strPrjID = "";
			strLidarID = -1;
			strPclMemo = "";
            pCrdData = NULL;
            pIntData = NULL;
            pClrData = NULL;
            pClassData = NULL;
            tPointNum = 0;
        }

        ~HD_POINT_CLOUD()
        {
            if (NULL != pCrdData)
            {
                delete pCrdData;
                pCrdData = NULL;
            }

            if (NULL != pIntData)
            {
                delete pIntData;
                pIntData = NULL;
            }

            if (NULL != pClrData)
            {
                delete pClrData;
                pClrData = NULL;
            }

            if (NULL != pClassData)
            {
                delete pClassData;
                pClassData = NULL;
            }
        }

        // 重载复制操作符
        HD_POINT_CLOUD& operator = (const HD_POINT_CLOUD& pointCloud)
        {
            if (this != &pointCloud)
            {
                strLevelID = pointCloud.strLevelID;
                strParcelID = pointCloud.strParcelID;
                nZone = pointCloud.nZone;
                strPrjID = pointCloud.strPrjID;
                strLidarID = pointCloud.strLidarID;
                dXMIN = pointCloud.dXMIN;
                dYMIN = pointCloud.dYMIN;
                dZMIN = pointCloud.dZMIN;
                dXMAX = pointCloud.dXMAX;
                dYMAX = pointCloud.dYMAX;
                dZMAX = pointCloud.dZMIN;

                // 浅拷贝
                pCrdData = pointCloud.pCrdData;
                pIntData = pointCloud.pIntData;
                pClrData = pointCloud.pClrData;
                pClassData = pointCloud.pClassData;
                tGatherTime = pointCloud.tGatherTime;
                strPclMemo = pointCloud.strPclMemo;
                tPointNum = pointCloud.tPointNum;
                hasData = pointCloud.hasData;
            }

            return *this;
        }

		// 解析strParcelID，获取层级编号、块集编号、块编号、包编号，由于strParcelID长度可变，可能造成块编号、包编号不存在情况，不存在均返回-1
		bool SerialParcelID(int& levelID,int& blockSetID,int& blockID,int& parcelID)
		{
			//// 条件判断
			//if (strParcelID == "")
			//{
			//	return false;
			//}

			// 初始化
			levelID = -1;
			blockSetID = -1;
			blockID = -1;
			parcelID = -1;

			// 由于strParcelID长度可变，由"-"个数进行区分，第一个对应层级，第二个对应块集，
			// 第三个对应块，第四个对应包
			int iZone = 0;
			int field = 0;
			int count = 0;
			char* strTmp = const_cast<char*>(strParcelID.c_str());
			while (*strTmp != '\0')
			{
				if (*strTmp == '-')
				{
					count++;
				}
				strTmp++;
			}

			if (count == 2) // 块集
			{
				field = sscanf_s(strParcelID.data(),"%d-%d-%d",&iZone,&levelID,&blockSetID);
				return field >= 3;
			}
			else if ( count == 3) // 块
			{
				field = sscanf_s(strParcelID.data(),"%d-%d-%d-%d",&iZone,&levelID,&blockSetID,&blockID);
				return field >= 4;
			}
			else if (count == 4) // 包
			{
				field = sscanf_s(strParcelID.data(),"%d-%d-%d-%d-%d",&iZone,&levelID,&blockSetID,&blockID,&parcelID);
				return field >= 5;
			}

			return false;
		}

        int strLevelID;                     //层级ID, 索引值, 非空
        string strParcelID;                 //切片名称, 索引值, 非空, 唯一
        int nZone;                          //投影代号, 索引值, 非空
        string strPrjID;                    //工程ID, 索引值
        int strLidarID;                     //扫描头编号
        double dXMIN;                       //XMIN, 索引值
        double dYMIN;                       //YMIN, 索引值
        double dZMIN;                       //ZMIN, 索引值
        double dXMAX;                       //XMAX, 索引值
        double dYMAX;                       //YMAX, 索引值
        double dZMAX;                       //ZMAX, 索引值
        HdPointXYZ* pCrdData;               //坐标数据, Blob
        HdIntensity* pIntData;              //强度数据, Blob
        HdPtColor* pClrData;                //颜色数据, Blob, 可选
        HdPtClass* pClassData;              //分类数据, Blob, 可选
        HDTIME tGatherTime;                 //采集时间, 用于区分版本
        string strPclMemo;                  //备注信息
        hd::u32 tPointNum;                  //点个数
        bool hasData;                       //是否含有点(有则不往下细分, 无则继续划分)
    };

    // POS文件数据类型
    struct HD_POS
    {
        // 构造函数
        HD_POS()
        {
			strPosID = "";
			strMemo = "";
        }

		// 重载赋值运算符
        HD_POS& operator = (const HD_POS& pos)
        {
            if (this != &pos)
            {
                strPosID = pos.strPosID;
                tStartTime = pos.tStartTime;
                tEndTime = pos.tEndTime;
                strMemo = pos.strMemo;
            }

            return *this;
        }

        string strPosID;                    // Pos标识, 索引值
        HDTIME tStartTime;                  // 开始时间
        HDTIME tEndTime;                    // 结束时间
        string strMemo;                     // 备注信息
    };

}
