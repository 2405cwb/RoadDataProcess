 
#pragma once
#include "hnapplication_global.h"
#include <QObject>
#include <QVector>
#include <vector>
#include "../hnCommon/hnRoadStruct.h"
namespace hnPro
{
	class hnProject;
}
class HNAPPLICATION_EXPORT hnDiseaseService: public QObject
{
	Q_OBJECT
public:
	explicit hnDiseaseService(QObject*parent = 0 );

	void setProject(hnPro::hnProject* project);

	//清空缓存 切换工程 增删改病害后调用
	void invalidateCache();

	//确保全局病害缓存已加载
	void ensureAllDiseaseCache();


	//获取当前视野病害
	void getRoadDiseasesInRange(double beginMile, double endMile, QVector<hnCommon::hnRoadDiseaseInfo>&result);

	void getStreetDiseaseInRange(double beginMile, double endMile, QVector<hnCommon::hnRoadDiseaseInfo>& result);

	//获取所有病害 病害列表  仅浏览病害模式用
	 QVector<hnCommon::hnRoadDiseaseInfo>& getAllDiseases();

	//获取所有景观病害
	 QVector<hnCommon::hnRoadDiseaseInfo> getAllStreetDiseases();

	 QVector<hnCommon::hnRoadDiseaseInfo> getAllRoadDiseases();

	 QVector<hnCommon::hnRoadDiseaseInfo> getAllDesignDiseases();



	//添加病害
	bool addDisease(
		hnCommon::hnRoadDiseaseInfo& disease);


	//批量添加指定类型病害
	bool addDataAffairs(QString tableName, bool write, QVector<hnCommon::hnRoadDiseaseInfo>& diseases);

	//删除病害
	bool deleteOneDisease( hnCommon::hnRoadDiseaseInfo&disease);

	//删除指定类型病害
	bool deleteAllTargetTypeDisease(QString stand,int  drawType);

	//删除所有病害
	bool deleteAllDiseases();



	//更新病害
	bool updateDisease( 
		hnCommon::hnRoadDiseaseInfo&disease);

	//从缓存中按照ID找病害
	bool findDisease(int diseaseId, const QString&tableName, hnCommon::hnRoadDiseaseInfo&result);

	//找下一个病害,仅浏览病害模式用
	bool findNextDisease(double currentMile, hnCommon::hnRoadDiseaseInfo& result);

	//找上一个病害
	bool findPreviousDisease(double currentMile, hnCommon::hnRoadDiseaseInfo& result);


signals:
	//任意病害变化 2d/3d用这个刷新视图
	void diseaseChanged();

	//病害列表用这些 做增量刷新
	void diseaseAdded(const hnCommon::hnRoadDiseaseInfo&disease);
	void diseaseDeleted(const hnCommon::hnRoadDiseaseInfo&disease);
	void diseaseUpdated(const hnCommon::hnRoadDiseaseInfo&disease);

	//大范围变化 如切换工程，批量导入，批量合并
	void diseaseReset();


private:
	bool isDiseaseInMileRange(const hnCommon::hnRoadDiseaseInfo& disease, double beginMile, double endMile,int split = 10) const;

	QVector<hnCommon::hnRoadDiseaseInfo> getAllDisease();

	//0 路面病害
	 QVector<hnCommon::hnRoadDiseaseInfo> getAllTypeDiseases(bool isRoad);


	 bool isSameDisease(const hnCommon::hnRoadDiseaseInfo& a, const hnCommon::hnRoadDiseaseInfo& b) const;

	 int findDiseaseIndexInCache(const hnCommon::hnRoadDiseaseInfo& disease) const;

	struct DiseaseRangeIndexEntry
	{
		double beginMile;
		double endMile;
		int cacheIndex;
	};

	void invalidateDiseaseRangeIndex();
	void rebuildDiseaseRangeIndex();
	void getDiseasesInRange(double beginMile, double endMile, double margin,
		int diseaseTypeFilter, QVector<hnCommon::hnRoadDiseaseInfo>& result);
private:

	hnPro::hnProject* m_project;

	bool m_allDiseaseCacheValid;

	QVector<hnCommon::hnRoadDiseaseInfo> m_allDiseaseCache;
	bool m_diseaseRangeIndexValid;
	QVector<DiseaseRangeIndexEntry> m_diseaseRangeIndex;
	QVector<double> m_diseaseRangePrefixMaxEnd;
 


};

