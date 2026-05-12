#include "hnDiseaseService.h"
#include "hnDataManager.h"
#include "../hnProject/hnProject.h"
#include "../hnDataTable/hnDBSqlite.h"
#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>

hnDiseaseService::hnDiseaseService(QObject* parent)
	: QObject(parent)
{
	m_project = 0;
	m_allDiseaseCacheValid = false;
}

void hnDiseaseService::setProject(hnPro::hnProject* project)
{
	if (m_project != project)
	{
		m_project = project;
		invalidateCache();
		qDebug() << "[DiseaseService] SetProject:"
			<< m_project
			<< "db:"
			<< (m_project ? m_project->getDB() : nullptr)
			<< "db open"
			<< (m_project&&m_project->getDB() ? m_project->getDB()->isOpen() : false);
	}
}

void hnDiseaseService::invalidateCache()
{
	m_allDiseaseCacheValid = false;
	m_allDiseaseCache.clear();
}

void hnDiseaseService::ensureAllDiseaseCache()
{
	if (m_allDiseaseCacheValid)
	{
		return;
	}

	if (m_project == 0)
	{
		return;
	}

	QElapsedTimer timer;
	timer.start();

	m_allDiseaseCache.clear();

	// 这里你需要根据你现有的 DB 接口换成真正读取全部病害的函数
	// 如果现在 getAllRoadDisease() 在 Widget 基类里，建议把那段逻辑搬到这里。 


	 
	m_allDiseaseCache = getAllRoadDisease();

	m_allDiseaseCacheValid = true;

	qDebug() << "[DiseaseService] load all diseases:"
		<< timer.elapsed()
		<< "ms, count:"
		<< m_allDiseaseCache.size();
}

const QVector<hnCommon::hnRoadDiseaseInfo>& hnDiseaseService::getAllDiseases()
{
	ensureAllDiseaseCache();
	return m_allDiseaseCache;
}

void hnDiseaseService::getDiseasesInRange(
	double beginMile,
	double endMile,
	std::vector<hnCommon::hnRoadDiseaseInfo>& result)
{
	ensureAllDiseaseCache();

	result.clear();

	QElapsedTimer timer;
	timer.start();

	for (int i = 0; i < m_allDiseaseCache.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = m_allDiseaseCache.at(i);

		if (isDiseaseInMileRange(disease, beginMile, endMile))
		{
			result.push_back(disease);
		}
	}

	qDebug() << "[DiseaseService] filter diseases:"
		<< timer.elapsed()
		<< "ms, result:"
		<< result.size();
}

bool hnDiseaseService::isDiseaseInMileRange(
	const hnCommon::hnRoadDiseaseInfo& disease,
	double beginMile,
	double endMile) const
{
	double viewBegin = beginMile;
	double viewEnd = endMile;

	if (viewBegin > viewEnd)
	{
		std::swap(viewBegin, viewEnd);
	}

	double diseaseBegin = disease.dDmiStart;
	double diseaseEnd = disease.dDmiEnd;

	if (diseaseBegin <= 0 && diseaseEnd <= 0)
	{
		diseaseBegin = disease.dMileage;
		diseaseEnd = disease.dMileage;
	}

	if (diseaseBegin > diseaseEnd)
	{
		std::swap(diseaseBegin, diseaseEnd);
	}

	const double margin = 10.0;
	viewBegin -= margin;
	viewEnd += margin;

	return diseaseEnd >= viewBegin && diseaseBegin <= viewEnd;
}

QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllRoadDisease()
{
		 //获取所有病害
		 if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
		 {
			 return  QVector<hnRoadDiseaseInfo>();
		 }
		 QVector<hnRoadDiseaseInfo> allDiseaseInfos;
	
		 
		 QString standard = HnProjectEnums::roadTypeEnumToQString(m_project->getBaseStandard());
		 if (PROJECT_TYPE::PROJECT_23D_TYPE == m_project->getProjectType() ||
			 PROJECT_TYPE::PROJECT_2D_TYPE == m_project->getProjectType())
		 {
	
			 // 二三维病害
	
			 QVector<hnMile> miles = m_project->getCurrentMileVector();
			 auto setinfo = m_project->getCurProSetInfo();
			 if (setinfo.nDrawType == 2)
			 {
				m_project->getDB()->
					 m_diseaseTable.readDesignDiseases(standard, 0, m_project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
			 }
			 else
			 {
				 m_project->getCurDB()->m_diseaseTable.readRoadDiseaseData(setinfo, miles, allDiseaseInfos, m_project->getCurrentMarkVector(), m_project->getRoadSpace());
			 }
		 }
		 else
		 {
			 std::vector<hnRoadDiseaseInfo> diseases3d;
			 // 纯三维病害
			 const double projectBeginEncoderMile = 0;
			 const double projectEndEncoderMile = m_project->getCurProSetInfo().dEndEnclMile;
			 auto xxx = m_project->getCurProSetInfo();
	
			 if (m_project->getCurProSetInfo().nDrawType == 2)
			 {
				 m_project->getDB()->
					 m_diseaseTable.readDesignDiseases(standard, 0, m_project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
			 }
			 else
			 {
				 m_project->getCurDB()
					 ->m_diseaseTable.read3dRoadDiseaseData(standard, projectBeginEncoderMile, projectEndEncoderMile, diseases3d);
				 allDiseaseInfos = QVector<hnRoadDiseaseInfo>::fromStdVector(diseases3d);
			 }
		 }
	
		 return allDiseaseInfos;
}

bool hnDiseaseService::addDisease(
	const hnCommon::hnProjectSetInfo& projectInfo,
	hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->m_diseaseTable.writeSingleDatas(
		projectInfo,
		disease
	);

	if (ok)
	{
		invalidateCache();
		emit diseaseChanged();
	}

	return ok;
}

bool hnDiseaseService::deleteDisease( hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->m_diseaseTable.deleteDisease(disease);

	if (ok)
	{
		invalidateCache();
		emit diseaseChanged();
	}

	return ok;
}

bool hnDiseaseService::updateDisease(
	const hnCommon::hnProjectSetInfo& projectInfo,
	hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->m_diseaseTable.updaetSingleDataInfo( disease);

    
	if (ok)
	{
		invalidateCache();
		emit diseaseChanged();
	}

	return ok;
}

bool hnDiseaseService::findDisease(
	int diseaseId,
	const QString& tableName,
	hnCommon::hnRoadDiseaseInfo& result)
{
	ensureAllDiseaseCache();

	for (int i = 0; i < m_allDiseaseCache.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = m_allDiseaseCache.at(i);

		if (disease.nID == diseaseId &&
			QString::fromLocal8Bit(disease.strDiseaseTableName) == tableName)
		{
			result = disease;
			return true;
		}
	}

	return false;
}

bool hnDiseaseService::findNextDisease(
	double currentMile,
	hnCommon::hnRoadDiseaseInfo& result)
{
	ensureAllDiseaseCache();

	bool found = false;
	double minDistance = 999999999.0;

	for (int i = 0; i < m_allDiseaseCache.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = m_allDiseaseCache.at(i);

		double mile = disease.dMileage;

		if (mile <= currentMile)
		{
			continue;
		}

		double distance = mile - currentMile;

		if (distance < minDistance)
		{
			minDistance = distance;
			result = disease;
			found = true;
		}
	}

	return found;
}

bool hnDiseaseService::findPreviousDisease(
	double currentMile,
	hnCommon::hnRoadDiseaseInfo& result)
{
	ensureAllDiseaseCache();

	bool found = false;
	double minDistance = 999999999.0;

	for (int i = 0; i < m_allDiseaseCache.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = m_allDiseaseCache.at(i);

		double mile = disease.dMileage;

		if (mile >= currentMile)
		{
			continue;
		}

		double distance = currentMile - mile;

		if (distance < minDistance)
		{
			minDistance = distance;
			result = disease;
			found = true;
		}
	}

	return found;
}