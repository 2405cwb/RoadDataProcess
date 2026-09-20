#pragma once
#include <QString>
#include <functional>
#include <QVector>
#include <vector>
#include "../hnCommon/hnRoadStruct.h"
namespace hnPro { class hnProject; }

class hnRoadDiseaseImageExporter
{
public:
    struct Result
    {
        bool success;
        int exported;
        int skipped;
        QString outputPath;
        QString error;
        Result() : success(false), exported(0), skipped(0) {}
    };
    Result exportProjects(const std::vector<hnPro::hnProject*>& projects, const QString& outputRoot,
        const std::function<bool(int, int, const QString&)>& progress) const;
private:
    bool exportProject(hnPro::hnProject* project, const QString& root, int& exported, int& skipped,
        QString& error, const std::function<bool(int, int, const QString&)>& progress, int& progressIndex, int total) const;
    QString imagePath(hnPro::hnProject* project, double dmi) const;
    QString safeName(const QString& value) const;
};
