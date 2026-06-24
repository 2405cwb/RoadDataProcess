#include "hnDiseaseSumAreaCaculate.h"



hnDiseaseSumAreaCaculate::hnDiseaseSumAreaCaculate(QObject *parent)
	: QObject(parent)
{
}

hnDiseaseSumAreaCaculate::~hnDiseaseSumAreaCaculate()
{
}

QVector<double> hnDiseaseSumAreaCaculate::caculateSumArea(const QVector<hnCommon::hnRoadDiseaseInfo> & diseases,bool mutWeight,
	const HnProjectEnums::StandardParmTypeEnum standard, int roadSurfaceType, hnPro::hnProject* project)
{
	QVector<double> result;

	//异常处理，如果路面材质不符合标准，就返回
	if (roadSurfaceType < 0 || roadSurfaceType >= 2)
	{
		return result;
	}

	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->
		getProjectRoadDiseaseNames(project, standard, roadSurfaceType);

	QMap<QString, double> disMsg;
	for (int i = diseaseSetInfos.size() - 1; i >= 0; --i)
	{

		auto disSetinfo = diseaseSetInfos.at(i);
		QString dbName = QString::fromLocal8Bit(disSetinfo.strDiseaseTypeName);
		if (!disMsg.contains(dbName))
		{
			disMsg[dbName] = 0;
		}
	}

	for (int t = 0; t < diseases.count(); ++t)
	{
		auto nowDis = diseases.at(t);
		QString disName = QString::fromLocal8Bit(nowDis.strDisName);

		if (disMsg.contains(disName)&&nowDis.nRSurfaceType == roadSurfaceType)
		{
			//统计表不变 false 汇总表除true

			//disMsg[disName] += nowDis.dArea / nowDis.diseaseWeight;
			if (mutWeight)
			{
				if (nowDis.diseaseWeight == 0 )
				{
				 
					if (project->getCurProSetInfo().nDrawType== 0|| project->getCurProSetInfo().nDrawType == 2)
					{
						disMsg[disName] += nowDis.dRealLen * nowDis.dReaWidth;

					}
					else if (project->getCurProSetInfo().nDrawType ==1)
					{
						disMsg[disName] += nowDis.vec2dRect.size()*0.01;
					}


				}
				else
				{
					disMsg[disName] += nowDis.dArea / nowDis.diseaseWeight;

				}

			}
			else
			{
				disMsg[disName] += nowDis.dArea;
			}
			 
				
		}
	}

	//按照固定顺序添加到结果中
	for (auto info : qAsConst(diseaseSetInfos))
	{
		result.append(disMsg[QString::fromLocal8Bit(info.strDiseaseTypeName)]);
	}

	return result;
}

bool hnDiseaseSumAreaCaculate::isAllZero(const QVector<double>& vector)
{
	for (auto element : qAsConst(vector))
	{
		if (element != 0)
		{
			return false;
		}
	}
	return true;
}
