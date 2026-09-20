#include "hnDiseaseSampleExporter.h"
#include "../hnDataTable/hnDBSqlite.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <sqlite3.h>

bool hnDiseaseSampleExporter::exportDatabase(hnDBSqlite* database, const QString& projectRoot,
    bool deleted, QString& outputPath, qint64& diseaseCount, QString& error)
{
    // 临时目录与最终目录同盘，只有完整备份和筛选均成功才发布结果。
    outputPath.clear();
    error.clear();
    diseaseCount = 0;
    if (!database || !database->isOpen() || projectRoot.isEmpty() || !QDir(projectRoot).exists())
    {
        error = QStringLiteral("当前工程或成果数据库不可用。");
        return false;
    }
    QTemporaryDir staging(QDir(projectRoot).filePath(QStringLiteral(".disease-sample-XXXXXX")));
    if (!staging.isValid())
    {
        error = QStringLiteral("无法在工程根目录创建导出文件夹，请检查写入权限和磁盘空间。");
        return false;
    }
    QString qstrName = QFileInfo(QString::fromLocal8Bit(database->getDBPath())).fileName();
    QString qstrCopy = QDir(staging.path()).filePath(qstrName);
    if (!database->backupDatabase(qstrCopy))
    {
        error = QStringLiteral("完整备份成果数据库失败，未生成导出结果。");
        return false;
    }
    if (!filterCopy(qstrCopy, deleted, diseaseCount, error))
    {
        return false;
    }
    QString qstrFolder = QStringLiteral("路面人工病害_");
    if (deleted)
    {
        qstrFolder = QStringLiteral("路面负样本_");
    }
    qstrFolder += QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmsszzz_")) +
        QUuid::createUuid().toString().mid(1, 8);
    QString qstrDestination = QDir(projectRoot).filePath(qstrFolder);
    if (!QDir().rename(staging.path(), qstrDestination))
    {
        error = QStringLiteral("发布导出目录失败，未生成导出结果。");
        return false;
    }
    staging.setAutoRemove(false);
    outputPath = QDir(qstrDestination).filePath(qstrName);
    return true;
}

bool hnDiseaseSampleExporter::filterCopy(const QString& databasePath, bool deleted,
    qint64& diseaseCount, QString& error)
{
    // SQLite 直接处理副本中的原始字段，避免历史本地编码文本被重新编码。
    diseaseCount = 0;
    error.clear();
    sqlite3* database = nullptr;
    if (sqlite3_open_v2(databasePath.toUtf8().constData(), &database, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK)
    {
        error = QStringLiteral("无法打开导出数据库副本。");
        if (database) sqlite3_close(database);
        return false;
    }
    sqlite3_busy_timeout(database, 5000);
    bool ok = sqlite3_exec(database, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) == SQLITE_OK;
    QStringList tables;
    sqlite3_stmt* statement = nullptr;
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'", -1, &statement, nullptr) == SQLITE_OK;
    }
    if (ok)
    {
        int state = SQLITE_ROW;
        while ((state = sqlite3_step(statement)) == SQLITE_ROW)
        {
            tables.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0))));
        }
        ok = state == SQLITE_DONE;
    }
    sqlite3_finalize(statement);
    int tableCount = 0;
    for (const QString& table : tables)
    {
        if (!ok) break;
        QString qstrQuoted = table;
        qstrQuoted.replace('"', QStringLiteral("\"\""));
        qstrQuoted = '"' + qstrQuoted + '"';
        statement = nullptr;
        QByteArray sql = (QStringLiteral("PRAGMA table_info(") + qstrQuoted + ')').toUtf8();
        ok = sqlite3_prepare_v2(database, sql.constData(), -1, &statement, nullptr) == SQLITE_OK;
        QSet<QString> columns;
        if (ok)
        {
            int state = SQLITE_ROW;
            while ((state = sqlite3_step(statement)) == SQLITE_ROW)
            {
                columns.insert(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))).toLower());
            }
            ok = state == SQLITE_DONE;
        }
        sqlite3_finalize(statement);
        // 遍历实际库表，包含旧规范遗留病害表，不依赖当前界面的显示过滤。
        if (!ok || !columns.contains(QStringLiteral("diseasetype")) || !columns.contains(QStringLiteral("addfile4"))) continue;
        tableCount++;
        QString qstrCondition = QStringLiteral("COALESCE(AddFile4,'') IN ('','0')");
        if (deleted)
        {
            qstrCondition = QStringLiteral("AddFile4 IN ('2','3')");
        }
        qstrCondition = QStringLiteral("COALESCE((DiseaseType=0 AND %1),0)").arg(qstrCondition);
        sql = QStringLiteral("DELETE FROM %1 WHERE NOT (%2)").arg(qstrQuoted, qstrCondition).toUtf8();
        ok = sqlite3_exec(database, sql.constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
        if (!ok) break;
        statement = nullptr;
        sql = QStringLiteral("SELECT COUNT(*) FROM %1").arg(qstrQuoted).toUtf8();
        ok = sqlite3_prepare_v2(database, sql.constData(), -1, &statement, nullptr) == SQLITE_OK;
        if (ok)
        {
            ok = sqlite3_step(statement) == SQLITE_ROW;
            if (ok) diseaseCount += sqlite3_column_int64(statement, 0);
        }
        sqlite3_finalize(statement);
    }
    if (tableCount == 0)
    {
        ok = false;
        error = QStringLiteral("成果库中没有可识别的病害表，未生成导出结果。");
    }
    if (ok) ok = sqlite3_exec(database, "COMMIT", nullptr, nullptr, nullptr) == SQLITE_OK;
    if (!ok)
    {
        if (error.isEmpty()) error = QStringLiteral("筛选导出数据库失败：%1").arg(QString::fromUtf8(sqlite3_errmsg(database)));
        sqlite3_exec(database, "ROLLBACK", nullptr, nullptr, nullptr);
        diseaseCount = 0;
    }
    sqlite3_close(database);
    return ok;
}
