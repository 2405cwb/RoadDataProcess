#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace hnPro
{
    class hnProject;
}

/**
 * @brief 工程诊断问题严重级别。
 */
enum ProjectDiagnosticSeverity
{
    ProjectDiagnosticInfo,
    ProjectDiagnosticWarning,
    ProjectDiagnosticError
};

/**
 * @brief 单条工程诊断结果。
 */
struct ProjectDiagnosticIssue
{
    ProjectDiagnosticSeverity severity;
    QString code;
    QString message;
    QString path;
    QString projectName;
};

/**
 * @brief 工程诊断结果集合。
 */
struct ProjectDiagnosticResult
{
    QVector<ProjectDiagnosticIssue> issues;
    int errorCount;
    int warningCount;
    int infoCount;
};

/**
 * @brief 负责工程只读检查和最小诊断包导出。
 *
 * 不修改源工程配置、数据库或采集文件。诊断包仅复制复现常见打开、查看和出表问题所需的数据。
 */
class hnProjectDiagnosticService
{
public:
    hnProjectDiagnosticService();

    /**
     * @brief 检查一个已打开工程的数据完整性。
     * @param project 待检查的工程。
     * @return 包含错误、警告和信息的检查结果。
     */
    ProjectDiagnosticResult checkProject(hnPro::hnProject* project);

    /**
     * @brief 将检查结果写入诊断目录。
     * @param result 要写入的检查结果。
     * @param outputDirectory 接收报告的目录。
     * @param reportPath 用于接收生成的报告路径。
     * @return 写入成功返回 true，否则返回 false。
     */
    bool writeReport(
        const ProjectDiagnosticResult& result,
        const QString& outputDirectory,
        QString& reportPath);
    bool writeManifest(
        const ProjectDiagnosticResult& result,
        const QString& outputDirectory,
        QString& manifestPath);

    /**
     * @brief 判断二维工程是否包含可导出的车辙原始数据。
     * @param project 当前工程。
     * @param cameraDirectory 用于接收根目录 camera0 的完整路径。
     * @return camera0/data 下存在文件时返回 true。
     */
    bool hasRawRutData(
        hnPro::hnProject* project,
        QString& cameraDirectory) const;

    /**
     * @brief 导出当前工程的最小诊断包。
     * @param project 当前工程。
     * @param targetDirectory 用户选择的输出父目录。
     * @param result 用于接收检查与导出结果。
     * @param packagePath 用于接收完成的诊断包目录。
     * @param includeRawRutData 是否复制二维工程根目录下的 camera0。
     * @return 导出成功返回 true，否则返回 false。
     */
    bool exportDiagnosticPackage(
        hnPro::hnProject* project,
        const QString& targetDirectory,
        ProjectDiagnosticResult& result,
        QString& packagePath,
        bool includeRawRutData);

private:
    void checkDatabaseTables(class hnDBSqlite* database, ProjectDiagnosticResult& result);
    void addIssue(
        ProjectDiagnosticResult& result,
        ProjectDiagnosticSeverity severity,
        const QString& code,
        const QString& message,
        const QString& path = QString());
    void checkRequiredFile(
        ProjectDiagnosticResult& result,
        const QString& filePath,
        const QString& code,
        const QString& description,
        bool required);
    void checkImageChannel(
        ProjectDiagnosticResult& result,
        const QString& channelName,
        const QStringList& imagePaths,
        int interval,
        double expectedLength);
    void check2DProject(
        hnPro::hnProject* project,
        ProjectDiagnosticResult& result);
    void checkImageSequenceChannel(
        ProjectDiagnosticResult& result,
        const QString& project2DPath,
        const QString& channelPath);
    void check3DProject(
        hnPro::hnProject* project,
        ProjectDiagnosticResult& result);
    bool copyFile(
        const QString& sourcePath,
        const QString& destinationPath,
        ProjectDiagnosticResult& result,
        bool required);
    bool copyDirectory(
        const QString& sourcePath,
        const QString& destinationPath,
        ProjectDiagnosticResult& result,
        bool required);
    void copyImageSamples(
        const QString& channelName,
        const QStringList& imagePaths,
        const QString& destinationDirectory,
        ProjectDiagnosticResult& result);
    QString buildPackageDirectory(const QString& targetDirectory, const QString& projectName) const;
};
