#pragma once

#include <QString>
class hnDBSqlite;

// 完整备份成果库后筛选路面样本，绝不修改源数据库。
class hnDiseaseSampleExporter
{
public:
    bool exportDatabase(hnDBSqlite* database, const QString& projectRoot, bool deleted,
        QString& outputPath, qint64& diseaseCount, QString& error);
    // 仅操作调用方提供的副本；保留所有非病害表及数据库结构。
    bool filterCopy(const QString& databasePath, bool deleted, qint64& diseaseCount, QString& error);
};
