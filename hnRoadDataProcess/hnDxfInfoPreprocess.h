#pragma once

#include <QObject>
#include "xlsxdocument.h"
#include "xlsxworksheet.h"
#include "HnProjectEnums.h"
#include <QMessageBox>
#include <QApplication>
#include "hnOutputXR.h"
#include <QDir>
#include "hnOutputXRProvinceRoad.h"
using namespace QXlsx;
//路面病害面积统计表的所属类型
enum class ExcelType
{
	NoType,											//无类型
	DegreeRoad2018_BigFrame,						//等级公路2018 人工模式
	DegreeRoad2018_LittleFrame,					//等级公路2018 自动化模式
	City_BigFrame,
	LowVillage_BigFrame

};
//导出dxf之前预处理对话框中的信息，处理完交由hnDxfIO进行出图
class hnDxfInfoPreprocess : public QObject
{
	Q_OBJECT

public:
	hnDxfInfoPreprocess(QObject *parent = nullptr);
	~hnDxfInfoPreprocess();

public:
	/*
	*接口名：preprocess
	*功能说明：预处理dxf导出的信息，处理完交由hnDxfIO进行出图
	*参数1：isNationProvincialRoad是否为国省道
	*参数2：excel表格绝对路径的列表
	*参数3：分段开始里程
	*参数4：分段结束里程
	*参数5：输出路径
	*参数6：上下行 大于0从小到大，小于0从大到小
	*返回值 0：正常
	*返回值 -1：文件名字中有不含“路面病害面积统计表”的情况，导出失败
	*返回值 -2：文件中有打不开的情况，导出失败
	*返回值 -3：文件中有的文件不含“病害列表”sheet页
	*返回值 -4：文件中表格无预定格式，或者有多种格式
	*/
	int preprocess(bool isNationProvincialRoad, const QStringList &excelFileNames, 
		double beginMile, double endMile,const QString &exportPath,double roadWidth ,int direction);

private:
	//检查表格的名字
	bool checkExcelFileName(const QStringList &excelFileNames);
	//检查表格是不是可以打开
	bool checkExcelIsOpen(const QStringList &excelFileNames);
	//检查表格是不是含有“病害列表”sheet页
	bool checkExcelSheetName(const QStringList &excelFileNames);
	// 表格是否有类型以及检查表格类型是否一致  返回true正常，false异常
	bool checkExcelType(const QStringList &excelFileNames);

private:
	//加载表格类型
	void loadExcelTypes();

private:
	//加载等级公路2018 人工模式 病害面积统计表第二行的表头
	QStringList loadDegreeRoad2018_BigFrameHeader();
	//加载等级公路2018 自动化模式 病害面积统计表第二行的表头
	QStringList loadDegreeRoad2018_LittelFrameHeader();

	QStringList loadCity_BigFrameHeader();

	QStringList loadLowVillage_bigFrameHeader();

	//...

private:
	//导出dxf
	void exportDxf(const GridDisease_C &gridDisease,ExcelType type, bool isNationProvincialRoad,
		const QStringList &excelFileNames,const QString &exportPath, int direction);
private:
	//等级公路2018 人工模式 导出dxf
	void exportDegreeRoad2018_BigFrameDxf(const GridDisease_C &gridDisease,bool isNationProvincialRoad, 
		const QStringList &excelFileNames, const QString &exportPath, int direction);

	//等级公路2018 自动化模式 导出dxf
	void exportDegreeRoad2018_LittelFrameDxf(const GridDisease_C &gridDisease, bool isNationProvincialRoad,
		const QStringList &excelFileNames, const QString &exportPath, int direction);

	int getDiseaseType(QString type);

	bool setDiseases(const QString &exportPath,QStringList excelFileNames, int direction,QString& path,std::vector<Disease_C>& diseases,int & roadType);
private:
	//加载表格病害列表sheet页第二行的表头
	QStringList loadExcelHeader(const QString &fileName);

	//把里程转换成带k的 //3003.456  转换后 成了 K3+003.456
	QString convertRegionWithK(double region);

private:
	//表格类型 key为表格的第二行的表头，从左到右依次存放  value为表格类型
	QMap<QStringList, ExcelType> m_excelTypes;

	//当前病害表格类型
	ExcelType m_currentExcelType;

	
};
