#pragma once
#include <QObject>
#include <QVector>
#include <vector>
#include "../hnCommon/hnRoadStruct.h"
namespace hnPro
{
	class hnProject;
}
class hnDiseaseService: public QObject
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
	void getDiseasesInRange(double beginMile, double endMile, std::vector<hnCommon::hnRoadDiseaseInfo>&result);


	//获取所有病害 病害列表  仅浏览病害模式用
	const QVector<hnCommon::hnRoadDiseaseInfo>& getAllDiseases();

	//添加病害
	bool addDisease(const hnCommon::hnProjectSetInfo& projectInfo,
		hnCommon::hnRoadDiseaseInfo& disease);

	//删除病害
	bool deleteDisease( hnCommon::hnRoadDiseaseInfo&disease);

	//更新病害
	bool updateDisease(const hnCommon::hnProjectSetInfo&projectInfo,
		hnCommon::hnRoadDiseaseInfo&disease);

	//从缓存中按照ID找病害
	bool findDisease(int diseaseId, const QString&tableName, hnCommon::hnRoadDiseaseInfo&result);

	//找下一个病害,仅浏览病害模式用
	bool findNextDisease(double currentMile, hnCommon::hnRoadDiseaseInfo& result);

	//找上一个病害
	bool findPreviousDisease(double currentMile, hnCommon::hnRoadDiseaseInfo& result);


signals:
	void diseaseChanged();


private:
	bool isDiseaseInMileRange(const hnCommon::hnRoadDiseaseInfo& disease, double beginMile, double endMile) const;

	QVector<hnCommon::hnRoadDiseaseInfo> getAllRoadDisease();
private:

	hnPro::hnProject* m_project;

	bool m_allDiseaseCacheValid;

	QVector<hnCommon::hnRoadDiseaseInfo> m_allDiseaseCache;
 


};

