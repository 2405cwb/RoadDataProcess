#pragma once
#include "../hnApplication/hnDataManager.h"
#include "../hnConfigService/HnXRSettings.h"

// 只转换报表使用的病害副本，不回写病害服务或工程数据库。
class hnReportDiseaseSeverity
{
public:
    void apply(QVector<hnCommon::hnRoadDiseaseInfo>& diseases, hnPro::hnProject* project)
    {
        if (!project || HnXRSettings::getInstance()->roadDisDegreeExcel != 1) return;
        QMap<int, QVector<hnCommon::hnDiseaseSetInfo>> settings;
        for (auto& disease : diseases)
        {
            QString name = QString::fromLocal8Bit(disease.strDisName);
            if (!name.endsWith(QStringLiteral(".轻")) && !name.endsWith(QStringLiteral(".中"))) continue;
            name = name.left(name.lastIndexOf('.')) + QStringLiteral(".重");
            if (!settings.contains(disease.nRSurfaceType))
                settings[disease.nRSurfaceType] = hnApp::hnDataManager::getDataManager()->getProjectRoadDiseaseNames(
                    project, project->getBaseStandard(), disease.nRSurfaceType);
            bool found = false;
            for (const auto& target : settings[disease.nRSurfaceType])
            {
                if (QString::fromLocal8Bit(target.strDiseaseTypeName) != name) continue;
                strcpy_s(disease.strDisName, sizeof(disease.strDisName), target.strDiseaseTypeName);
                strcpy_s(disease.strDiseaseTableName, sizeof(disease.strDiseaseTableName), target.strDBTableName);
                disease.nLevel = target.nLevel;
                disease.diseaseWeight = target.fWidget;
                found = true;
                break;
            }
            if (!found)
            {
                const QString message = QStringLiteral("按重度出表时未找到对应配置：%1，保留该病害原程度。").arg(name);
                auto settings = HnXRSettings::getInstance();
                if (!settings->ExcelErrorMessageList.contains(message)) settings->ExcelErrorMessageList.append(message);
            }
        }
    }
};
