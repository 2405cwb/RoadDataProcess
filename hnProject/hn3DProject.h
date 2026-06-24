#ifndef _HN_3D_PROJECT_H_
#define _HN_3D_PROJECT_H_
#include "hnproject_global.h"
#include <QString>
#include <vector>
#include <QDebug>
#include "..\hnCommon\hn2dPointDef.h"
#include "..\hnCommon\hn3dPointDef.h"
#include "..\hnPavementCreate3d\hnPavementCamReader.h"
#include "..\hnPavementCreate3d\hnPavementImageInfo.h"
using namespace hnCommon;
using namespace std;

namespace hnPro
{
	class HNPROJECT_EXPORT hn3DProject
	{
	public:
		hn3DProject();
		~hn3DProject();

	public:
		// 初始化3D工程
		bool init(const QString& strProjectPath, const QString& strProjectName);

		// 设置当前工程
		bool setCurProject();

		// 清空工程
		void closeProject();

	public:
		// 获取点云灰度图路径
		QString getGreyImagePath() { return m_strGreyImage; }

		// 获取点云深度图路径
		QString getDepthImagePath() { return m_strDepthImage; }

		// 获取点云相对路径
		QString getXDCloudPath() { return m_strXDCloudPath; }

		// 获取绝对点云路径
		QVector<QString> getJDCloudPath() { return m_vecJDCloudPath; }

		// 获取绝对点云名称
		QVector<QString> getJDCloudName() { return m_vecJDCloudName; }
		
		// 获取相机参数
		QString getCamParamPath() { return m_strImageParam; }

		// 根据影像名称以及图片ROW坐标获取GPS时间  nImageRow = 图片高度 - y坐标 左下角为坐标系原点 控制点需要调用此函数
		double getGpsTimer(QString strImageName, int nImageRow);

		// 根据影像名称以及图片ROW坐标获取点云帧号  nImageRow = 图片高度 - y坐标 左下角为坐标系原点
		int getPcdFrame(QString strImageName, int nImageRow);

		// 根据GPS获取图像
		QString getImageByGps(double dGpsTimer);

		// 根据里程获取图像
		QString getImageByMile(double dMile);

		// 根据帧号获取图像
		QString getImageByIndex(int nIndex);

		// 获取工程路径
		QString getProjectPath() { return m_strProjectPath;}

		// 获取所有图像路径
		QVector<QString> getAllImage();

		// 根据影像名称获取里程  这个图片名字是不包含路径的
		double getMileByImage(const QString &strImageName);

		// 获取影像像素宽度
		int getImagePixelWidth();

		// 获取影像像素高度
		int getImagePixelHeight();

		// 获取影像横向比例 每个像素代表多少米
		double getImageWidthScale();

		// 获取影像纵向比例 每个像素代表多少米
		double getImageHeightScale();

		// 获取路面宽度
		double getRoadWidth();

		// 设置路面宽度
		void setRoadWidth(double width);

		/*
		* 函数名：get2DCoord
		* 函数功能：根据影像名称和像素坐标获取相对坐标 左下角为坐标系原点 
		* 参数1：[IN]3d的图片名字，不含RGB 或者GREY 
		* 参数2：[IN]图片里面的坐标，以左下角为坐标系
		* 参数3：[OUT] 输出的三维坐标，包含xyz
		* 备注： 控制点里面计算xyz要用这个
		*/
		bool get2DCoord(QString strImageName, hn2dPointI pixelCoord, hn3dPointD& pt3D);

		// 根据影像名称以及图片像素坐标获取三维坐标 左下角为坐标系原点
		bool get3DCoord(QString strImageName, hn2dPointI pixelCoord, hn3dPointD& pt3D);

	public:
		// 获取是否水平镜像
		bool getIsHMirrored();

		// 设置是否水平镜像
		void setIsHMirrored(bool mirrored);

		// 获取是否垂直镜像
		bool getIsVMirrored();

		// 设置是否垂直镜像
		void setIsVMirrored(bool mirrored);

		hnPavementCamReader * getPavementCamReader() { return m_pPcdReader; };
	private:
		//检查镜像配置文件，如果不存在则新建
		void checkMirroredFile(const QString &fileName);

	private:
		// 相对点云路径
		QString m_strXDCloudPath;

		// 绝对点云路径
		QVector<QString> m_vecJDCloudPath;

		// 绝对点云名称
		QVector<QString> m_vecJDCloudName;

		// 点云灰度图路径
		QString m_strGreyImage;

		// 点云深度图路径
		QString m_strDepthImage;

		// 相机参数
		QString m_strImageParam;

		// 相对点云对象
		hnPavementCamReader* m_pPcdReader;

		// 所有影像数据 这里面的图片名称是去掉 RGB或者GRAY的
		vector<PAVEMENT_IMAGE_INDEX> m_vecImageInfo;

		QMap<QString, double> m_vecImageInfoMap;
		// 工程路径
		QString m_strProjectPath;

		//配置文件名字，不含路径
		QString m_configFileName;
	};

}

#endif
