#pragma once

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QSqlRecord>
#include "..\hnApplication\hnDataManager.h"
#include "..\hnApplication\hn2d3dCoordinates.h"
#include "../hnProject/hnProject.h"
#include "../hnProject/hn2DProject.h"
#include "../hnProject/hn3DProject.h"
#include <QMessageBox>
#include "configService.h"
#include "HnProjectEnums.h"
#include "../hnDataTable/hnRoadDiseaseTable.h"
#include "../hnCommon/hnRoadStruct.h"
#include <QProgressDialog>
#include <QApplication>

#define TEST_TABLENAME "DisJL"

using namespace hnApp;

//从自动识别数据库里面提取的有效信息
struct AidcDisease
{
	QString roadStandard;		//公路等级
	int roadSurfaceType = 0;	//道路材质 0-沥青；1-水泥；2-砂石
	int drawType = 0;			//框选模式 0：人工模式  1：自动化模式
	int level = 0;				//病害等级 0 - 无；1 - 轻；2 - 中； - 3重
	int lenth = 0;				//像素长度 横向
	int width = 0;				//像素宽度 纵向
	/*
	* 病害坐标 人工模式自动化模式格式不同 
	* 人工模式 x,y 病害的左上角坐标，中间以逗号分割
	* 自动化模式 -病害自动化模式角标-病害自动化模式角标- （按从左到右 从上到下排的）
	*/
	QString coord;	
	QString tableName;			//病害表名
	double dmi = 0.0f;			//病害里程 0 2 4 6 8 ...
	double roadWidth = 0.0f;	//路面宽度	
	QString disMark = "";
};

//导入自动识别病害
class hnImportAidcDiseases : public QObject
{
	Q_OBJECT

public:
	//frameType ：框选类型  0：人工模式  1：自动化模式
	hnImportAidcDiseases(const int frameType, QWidget *parent);
	hnImportAidcDiseases(const int frameType, bool isMerge, bool isMap, QWidget *parent);

	~hnImportAidcDiseases();

public:
	//导入自动识别病害
	void import();

	//导入人工2d病害
	void import2dDisease();

	public:
	//加载数据库
	bool loadDb();

	//加载2d文本病害
	bool load2dDb();

	//把多个表中的病害 批量写入数据库
	void writeDb(QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases);

	public:
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> transformDiseases();

	hnCommon::hn3dPointWithMileI getHnPoint3dWithMileI(const QPoint & point, const double mile, bool hMirror, bool vMirror);

	//创造单个图片的自动化模式数组	这里的自动化模式是针对小的image的
	QVector<QRect> createSingleImageLittleFrameRect();

private:
	//转换自动识别病害信息为当前程序所用的、可写入成果数据库的信息  人工模式
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> transformBigFrameDiseases();

	//人工模式计算病害的中心里程
	double caculateBigFrameDiseaseCenterMile(const AidcDisease disease);

	//人工模式计算病害的开始里程
	double caculateBigFrameDiseaseBeginMile(const AidcDisease disease);

	//人工模式计算病害的结束里程
	double caculateBigFrameDiseaseEndMile(const AidcDisease disease);

	//计算某个rect的中心里程
	double caculateRectCenterMile(const QRect &rect, const double buttomMile);

	//根据自动识别的信息，创建人工模式病害数组
	QRect createBigFrameRect(const QString &coord, int width, int height);


	//根据病害的坐标矩形(单张图片的），创建人工模式写入数据库的坐标数组
	vector<hn2dRectI> generateLargeFrameHn2dRectVector(const QRect &rect, const double mile);

	//QPoint 转 hn2dPointWithMileI  这里的QPoint是指单张图片里面的Qpoint
	hnCommon::hn2dPointWithMileI getHnPoint2dWithMileI(const QPoint &point ,const double mile);

public:
	//人工模式 根据病害的坐标矩形，创建写入数据库的3d坐标数组
	vector<hn3dRectI> generateLargeFrameHn3dRectVector(const hn2dRectI &rect);

	//人工模式 获取hn3dPointWithMileI  point是二维单张图片的 二维转三维
	hnCommon::hn3dPointWithMileI getHnPoint3dWithMileI(const QPoint &point, const double mile);


private:
	//转换自动识别病害信息为当前程序所用的、可写入成果数据库的信息  自动化模式
	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> transformLittleFrameDiseases();

	//自动化模式计算病害的中心里程
	double caculateLittleFrameDiseaseCenterMile(const AidcDisease disease, QVector<QRect> rects);

	//自动化模式病害计算开始里程
	double caculateLittleFrameDiseaseBeginMile(const AidcDisease disease, QVector<QRect> rects);

	//自动化模式病害计算结束里程
	double caculateLittleFrameDiseaseEndMile(const AidcDisease disease, QVector<QRect> rects);

	//自动化模式病害计算结束里程

	////创造单个图片的自动化模式数组	这里的自动化模式是针对小的image的
	//QVector<QRect> createSingleImageLittleFrameRect();

	//自动化模式，根据自动识别病害的坐标创造QRect数组,异常返回空数组 第二个参数是单张图片的所有自动化模式
	QVector<QRect> createLittleFrameRects(const AidcDisease &disease,const QVector<QRect> rects);

	//自动化模式 根据单张图片QRect数组 创建可储存到数据库的vector<hn2dRectI>
	vector<hn2dRectI> createLittleFrame2dRectIVector(const QVector<QRect> rects, const double mile);

public:

	//自动化模式 根据单张图片QRect数组 创建可储存到数据库的vector<hn3dRectI> 映射用
	vector<hn3dRectI> createLittleFrame3dRectIVector(const QVector<QRect> rects, const double mile);

	// 合并多个病害时病害的里程由子病害里程决定
	vector<hn3dRectI> createLittleFrame3dRectIVector(hnCommon::hnRoadDiseaseInfo &newDisease);

private:
	//计算病害的数量
	int caculateDiseasesCount(QMap<QString, QVector<AidcDisease>> diseases);


private:
	//自动识别的病害信息
	//QVector<AidcDisease> m_diseases;

	//所有自动识别出来的病害信息
	QMap<QString, QVector<AidcDisease>> m_allAidcDiseases;

	//病害表名与病害类型的对应关系
	QMap<QString, QString> m_diseaseNameMap;


	//病害表名与病害完整信息对应关系
	QMap<QString, std::vector<hnDiseaseSetInfo>>m_diseaseInfoMap;

	//框选类型  0：人工模式  1：自动化模式
	int m_frameType;

	//是否映射
	bool m_isDiseaseMap;

	//是否合并病害
	bool m_isMerge;

private:
	QWidget *m_parent;

};
