#include "../hnDataTable/hnDataTable.h"
#include "../hnProject/hnProject.h"
#include "hnDiseaseService.h"
#include "hnDataManager.h"

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

	m_allDiseaseCache = getAllDisease();
	std::sort(m_allDiseaseCache.begin(), m_allDiseaseCache.end());

	m_allDiseaseCacheValid = true;

	qDebug() << "[DiseaseService] load all diseases:"
		<< timer.elapsed()
		<< "ms, count:"
		<< m_allDiseaseCache.size();
}

 QVector<hnCommon::hnRoadDiseaseInfo>& hnDiseaseService::getAllDiseases()
{
	ensureAllDiseaseCache();
	return m_allDiseaseCache;
}

 QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllStreetDiseases()
{
	return	getAllTypeDiseases(false);
}

 QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllRoadDiseases()
{
	return	getAllTypeDiseases(true);
}

 QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllDesignDiseases()
 {
	 //error
	 ensureAllDiseaseCache();
	 return m_allDiseaseCache;
 }

void hnDiseaseService::getRoadDiseasesInRange(
	double beginMile,
	double endMile,
	QVector<hnCommon::hnRoadDiseaseInfo>& result)
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

	/*qDebug() << "[DiseaseService] filterRoad diseases:"
		<< timer.elapsed()
		<< "ms, result:"
		<< result.size();*/
}

void hnDiseaseService::getStreetDiseaseInRange(double beginMile, double endMile, QVector<hnCommon::hnRoadDiseaseInfo>& result)
{
	ensureAllDiseaseCache();

	result.clear();

	QElapsedTimer timer;
	timer.start();
	QVector<hnCommon::hnRoadDiseaseInfo> diss = getAllStreetDiseases();
	for (int i = 0; i < diss.size(); ++i)
	{
		const hnCommon::hnRoadDiseaseInfo& disease = diss.at(i);

		if (isDiseaseInMileRange(disease, beginMile, endMile,0))
		{
			result.push_back(disease);
		}
	}

 
}

bool hnDiseaseService::isDiseaseInMileRange(
	const hnCommon::hnRoadDiseaseInfo& disease,
	double beginMile,
	double endMile, int split) const
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

	const double margin = split;
	viewBegin -= margin;
	viewEnd += margin;

	return diseaseEnd >= viewBegin && diseaseBegin <= viewEnd;
}

QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllDisease()
{
	//获取所有病害
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return  QVector<hnRoadDiseaseInfo>();
	}
	QVector<hnRoadDiseaseInfo> allDiseaseInfos;

	if (m_project == 0)
	{
		return allDiseaseInfos;
	}

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
				getDiseaseTable()->readDesignDiseases_Service(standard,
					0,
					m_project->getCurProSetInfo().dEndEnclMile,
					allDiseaseInfos);
		}
		else
		{
			hnDBSqlite * db = m_project->getCurDB();
			/*qDebug() << "[DiseaseService before  read]"
				<< "project" << m_project
				<< "db:" << db
				<< "dieaseTabel:" << &(db->getDiseaseTable()->)
				<< "table sqlite ptr:" << db->getDiseaseTable()->debugDbPtr()
				<< "sizeof(hnDBSqlite)" << sizeof(hnDBSqlite);

			db->getDiseaseTable()->debugIsDBOpen();*/

			db->getDiseaseTable()->readRoadDiseaseData_Service(setinfo,
				miles,
				allDiseaseInfos,
				m_project->getCurrentMarkVector(),
				m_project->getRoadSpace());
		}
		//景观病害
		QVector<hnRoadDiseaseInfo> streetDiseases;
		m_project->getCurDB()->getDiseaseTable()->
			readStreetData_Service(standard, miles, streetDiseases, m_project->getCurProSetInfo().nLineType, m_project->getRoadSpace());

		allDiseaseInfos += streetDiseases;
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
				getDiseaseTable()->readDesignDiseases_Service(standard,
					0,
					m_project->getCurProSetInfo().dEndEnclMile,
					allDiseaseInfos);
		}
		else
		{
			m_project->getCurDB()
				->getDiseaseTable()->read3dRoadDiseaseData_Service(standard, projectBeginEncoderMile, projectEndEncoderMile, diseases3d);
			allDiseaseInfos = QVector<hnRoadDiseaseInfo>::fromStdVector(diseases3d);
		}
	}

	return allDiseaseInfos;
}

 QVector<hnCommon::hnRoadDiseaseInfo> hnDiseaseService::getAllTypeDiseases(bool isRoad)
{
	ensureAllDiseaseCache();
	QVector<hnCommon::hnRoadDiseaseInfo> diss;
	for (int i = 0; i < m_allDiseaseCache.size(); ++i)
	{
		hnCommon::hnRoadDiseaseInfo& dis = m_allDiseaseCache[i];
		if (isRoad&& dis.ndiseaseType == 0)
		{
			diss.push_back(dis);

		}
		if (!isRoad&& dis.ndiseaseType != 0)
		{
			diss.push_back(dis);

		}

	}
	return diss;
}

 bool hnDiseaseService::isSameDisease(const hnCommon::hnRoadDiseaseInfo& a, const hnCommon::hnRoadDiseaseInfo& b) const
 {
	 return a.nID == b.nID && std::strcmp(a.strDiseaseTableName, b.strDiseaseTableName) == 0;
 }

 int hnDiseaseService::findDiseaseIndexInCache(const hnCommon::hnRoadDiseaseInfo& disease) const
 {
	 for (int i = 0 ; i < m_allDiseaseCache.size() ;++i)
	 {
		 if (isSameDisease(m_allDiseaseCache.at(i),disease))
		 {
			 return i;
		 }
	 }
	 return -1;
 }

bool hnDiseaseService::addDisease(
	hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}
	const hnCommon::hnProjectSetInfo& projectInfo = m_project->getCurProSetInfo();
	const bool traceLittleFrame =
		disease.nDrawType == 1 ||
		disease.vec2dRect.size() >= 128 ||
		disease.vec3dRect.size() >= 128;
	QElapsedTimer addTimer;
	QElapsedTimer stepTimer;
	if (traceLittleFrame)
	{
		addTimer.start();
		stepTimer.start();
	}
	//注意看看这个方法 会不会回填ID
	bool ok = m_project->getDB()->getDiseaseTable()->writeSingleDatas_Service(
		projectInfo,
		disease
	);
	const qint64 writeMs = traceLittleFrame ? stepTimer.restart() : 0;
	if (!ok)
	{
		if (traceLittleFrame)
		{
			#ifdef _DEBUG
			qDebug() << "HN_LITTLE_FRAME_PERF addDisease failed"
				<< "id=" << disease.nID
				<< "drawType=" << disease.nDrawType
				<< "vec2d=" << disease.vec2dRect.size()
				<< "vec3d=" << disease.vec3dRect.size()
				<< "writeMs=" << writeMs
				<< "totalMs=" << addTimer.elapsed();
			#endif
		}
		return false;
	}
	if (m_allDiseaseCacheValid)
	{
		// Database write is the source of truth after an add. Invalidate instead of
		// trusting the caller-filled ID, otherwise SDK views can miss the new disease.
		invalidateCache();
	}
	const qint64 cacheMs = traceLittleFrame ? stepTimer.restart() : 0;
	emit diseaseAdded(disease);
	emit diseaseChanged();
	const qint64 signalMs = traceLittleFrame ? stepTimer.elapsed() : 0;
	if (traceLittleFrame)
	{
		#ifdef _DEBUG
		qDebug() << "HN_LITTLE_FRAME_PERF addDisease"
			<< "id=" << disease.nID
			<< "drawType=" << disease.nDrawType
			<< "vec2d=" << disease.vec2dRect.size()
			<< "vec3d=" << disease.vec3dRect.size()
			<< "writeMs=" << writeMs
			<< "cacheMs=" << cacheMs
			<< "signalMs=" << signalMs
			<< "totalMs=" << addTimer.elapsed();
		#endif
	}
	return true;
}
bool hnDiseaseService::addDataAffairs(QString tableName, bool write, QVector<hnCommon::hnRoadDiseaseInfo>& diseases)
{
	if (m_project == 0)
	{
		return false;
	}
	//注意看看这个方法 会不会回填ID
	bool ok = m_project->getDB()->getDiseaseTable()->writeDataAffairs_Service(
		tableName.toLocal8Bit().data(), write,diseases.toStdVector() 
	);
	if (!ok)
	{
		return false;
	}
	if (m_allDiseaseCacheValid)
	{
		invalidateCache();
	}
	emit diseaseReset();
	return true;
}

bool hnDiseaseService::deleteOneDisease(hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->getDiseaseTable()->deleteDisease_Service(disease);
	if (!ok)
	{
		return false;
	}
	if (m_allDiseaseCacheValid)
	{
		for (int i = m_allDiseaseCache.size() -1; i>=0; --i)
		{
			if (isSameDisease(m_allDiseaseCache.at(i),disease))
			{
				m_allDiseaseCache.remove(i);
				break;
			}
		}
	}

	emit diseaseDeleted(disease);
	emit diseaseChanged();

	return ok;
}

bool hnDiseaseService::deleteAllTargetTypeDisease(QString stand, int drawType)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->getDiseaseTable()->deleteAllTargetDrawTypeDisease_Service(stand,drawType);
	if (!ok)
	{
		return false;
	}
	if (m_allDiseaseCacheValid)
	{
		invalidateCache();
	}

	emit diseaseReset(); 
	emit diseaseChanged();
	return ok;
}

bool hnDiseaseService::deleteAllDiseases()
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->getDiseaseTable()->deleteAllDisease_Service();
	if (!ok)
	{
		return false;
	}
	if (m_allDiseaseCacheValid)
	{
		invalidateCache();
	}

	emit diseaseReset();
	emit diseaseChanged();
	return ok;
}

bool hnDiseaseService::updateDisease(
	 
	hnCommon::hnRoadDiseaseInfo& disease)
{
	if (m_project == 0)
	{
		return false;
	}

	bool ok = m_project->getDB()->getDiseaseTable()->updaetSingleDataInfo_Service(disease);
	if (!ok)
	{
		return false;
	}

	 if (m_allDiseaseCacheValid)
	 {
		 int index = findDiseaseIndexInCache(disease);
		 if (index >=0)
		 {
			 m_allDiseaseCache[index] = disease;
		 }
		 else
		 {
			 invalidateCache();
		 }
	 }
	 emit diseaseUpdated(disease);
	 emit diseaseChanged();

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
