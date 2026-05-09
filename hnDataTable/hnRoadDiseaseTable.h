#ifndef _HN_ROAD_DISEASE_TABLE_H_
#define _HN_ROAD_DISEASE_TABLE_H_
#include "hnDBTable.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "hndatatable_global.h"
#include "hnMile.h"
using namespace hnCommon;


class HNDATATABLE_EXPORT hnRoadDiseaseTable : public hnDBTable
{
public:
	hnRoadDiseaseTable();
	virtual ~hnRoadDiseaseTable();

public:
	// 设置病害表名称列表
	void setDiseaseTableName(vector<string>& vecDiseaseTableName);
	//删除病害
	bool deleteDiseases(vector<hnRoadDiseaseInfo> &vecData);
	bool deleteDisease(hnRoadDiseaseInfo &vecData);

	//清空所有病害
	bool deleteAllDisease();
	bool deleteAllDisease(vector<string> allDiseaseTables);

	// 读取所有病害数据信息
	//bool readAllData(QVector<hnRoadDiseaseInfo>& vecData, char* strQuery = NULL);
public:
	//获取工程中所有符合打标,及工程属性的病害（包括景观病害）
	bool readAllDiseases(const hnProjectSetInfo& setInfo, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo);

	//供单独三维工程调用  
	bool read3dRoadDiseaseData(const QString& standard, double dmiStart,double dmiEnd ,  vector<hnRoadDiseaseInfo>&vecData);

	// 获得路面病害   miels::桩号表  vecData 返回病害表 line 上下行  roadYDistance：纵向距离  注意获得的病害面积并没有乘以权重
	bool readRoadDiseaseData(const hnProjectSetInfo& setInfo, const QVector<hnMile>& miles, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo, double roadYDistance);
	bool readRoadDiseaseData(const hnProjectSetInfo& setInfo, double dmiStart, double dmiEnd, QVector<hnRoadDiseaseInfo>&vecData, const QVector<hnMarkInfo>& markinfo, double roadYDistance);

	//获得区间内景观病害
	bool readStreetData(const QString& standard, const QVector<hnMile>& miles, QVector<hnRoadDiseaseInfo>&vecData, int line, double roadYDistance);

	// 读取设计模式病害
	void readDesignDiseases(const QString& standard, double beginEncoderMile, double endEncoderMile, QVector<hnRoadDiseaseInfo>&data);
	
		
	// 读取指定表的病害数据
	//未对道路标准与道路类型进行过滤处理
	/*bool readDataFromTableName(const QString& standard, const char* strTableName, vector<hnRoadDiseaseInfo>& vecData, char* strQuery = NULL);
*/
	// 获取某类型病害最大id
	virtual int getMaxID();

	// 获取某类型病害最大id 自增1
	int getMaxID(const string& strTableName);

	// 批量写入病害数据信息
	bool writeDatas(const hnProjectSetInfo& projectConfig, vector<hnRoadDiseaseInfo>& vecData);

	// 批量病害数据信息
	bool writeDataAffairs(const char* strTableName, bool bWrite, vector<hnRoadDiseaseInfo>& vecData, bool(*pProgress)(float fVal, const char* qstrName, bool bCancle) = NULL);

	// 写入病害数据信息
	bool writeSingleDatas(const hnProjectSetInfo& projectConfig, hnRoadDiseaseInfo& inData);

	//更新病害
	bool updaetSingleDataInfo(hnRoadDiseaseInfo& inData);

	//合并病害   将新病害添加到旧病害库，过滤掉相同病害
	bool mergeDatas(const hnProjectSetInfo& projectConfig, QVector<hnRoadDiseaseInfo>& newDiseases, QVector<hnRoadDiseaseInfo>& oldDiseases);

	//删除表数据
	bool deleteFormData(const char* strTableName);

	//检查数据库是否有传入的数据类型
	//0 人工模式
	//1 自动化模式
	//2 设计模式
	bool checkDiseaseExist(QString standard ,  int diseaseType);

	//删除所有 相应绘制类型的病害
	bool deleteAllTargetDrawTypeDisease(QString standard,int drawType);

public: 
	vector<string> GetAllDiseaseTableNames() { return m_vecDiseaseTableName; }

private:
	//裂纹病害号
	int m_nMaxId;

	// 病害表名
	vector<string> m_vecDiseaseTableName;
	//void handelMiles(const QVector<hnMile>& miles,  vector<hnRoadDiseaseInfo>& allDatas,QVector<hnRoadDiseaseInfo>&vecData, double  roadYDistance,int i,bool isStreet= false);

	//根据打标类型等对返回病害进行过滤
	// isLine  是否是线性病害
	void FilterOutDisrase(const vector<hnRoadDiseaseInfo>& allDisease, QVector<hnRoadDiseaseInfo>&returnDisease, const QVector<hnMarkInfo>& markinfo, const hnProjectSetInfo& setInfo,bool isLine);

	//设置删除病害  软删除
	void setDeleteDisease(hnRoadDiseaseInfo& dis);

	//设置新增病害
	void setAddDisease(hnRoadDiseaseInfo & dis);
};


#endif // _HN_RING_TABLE_H_
