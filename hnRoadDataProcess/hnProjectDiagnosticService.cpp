#include "hnProjectDiagnosticService.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QtAlgorithms>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegExp>
#include <QSettings>
#include <QTextStream>

#include "..\\hnProject\\hnProject.h"
#include "..\\hnProject\\hn2DProject.h"
#include "..\\hnDataTable\\hnDBSqlite.h"
#include "..\\hnCommon\\hnRoadTypeDef.h"
#include "..\\hnLogService\\logMgr.h"

namespace
{
    // 采集格式规定：除最后一个分包外，每个 Image_#### 分包固定保存 1000 张图像。
    const int kImageChunkFrameCount = 1000;

    struct ImageChunkDirectory
    {
        int index;
        QFileInfo info;
    };

    bool imageChunkDirectoryLessThan(const ImageChunkDirectory& left, const ImageChunkDirectory& right)
    {
        return left.index < right.index;
    }
}

hnProjectDiagnosticService::hnProjectDiagnosticService()
{
}

ProjectDiagnosticResult hnProjectDiagnosticService::checkProject(hnPro::hnProject* project)
{
    ProjectDiagnosticResult result;
    result.errorCount = 0;
    result.warningCount = 0;
    result.infoCount = 0;

    if (project == nullptr)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("PROJECT_NULL"),
            QStringLiteral("当前没有可检查的工程。"));
        return result;
    }

    const QString projectRoot = project->getAbsulotelyPath();
    if (projectRoot.isEmpty() || !QDir::isAbsolutePath(projectRoot))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("PROJECT_ROOT_INVALID"),
            QStringLiteral("工程根目录为空或不是绝对路径，无法创建检查报告。"), projectRoot);
        return result;
    }
    if (!QDir(projectRoot).exists())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("PROJECT_ROOT_MISSING"),
            QStringLiteral("工程根目录不存在。"), projectRoot);
        return result;
    }

    if (project->getCurDB() == nullptr)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_NOT_OPEN"),
            QStringLiteral("工程数据库未成功打开，无法核对里程、标桩和病害数据。"));
    }
    else if (!project->getCurDB()->isOpen())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_NOT_OPEN"),
            QStringLiteral("工程数据库对象存在但连接未打开，无法核对里程、标桩和病害数据。"));
    }
    else
    {
        checkDatabaseTables(project->getCurDB(), result);
    }

    const hnCommon::PROJECT_TYPE projectType = project->getProjectType();
    if (projectType == hnCommon::PROJECT_2D_TYPE || projectType == hnCommon::PROJECT_23D_TYPE)
    {
        check2DProject(project, result);
    }

    if (projectType == hnCommon::PROJECT_XD_3D_TYPE ||
        projectType == hnCommon::PROJECT_JD_3D_TYPE ||
        projectType == hnCommon::PROJECT_23D_TYPE)
    {
        check3DProject(project, result);
    }

    addIssue(result, ProjectDiagnosticInfo, QStringLiteral("CHECK_FINISHED"),
        QStringLiteral("工程检查完成：错误 %1 项，警告 %2 项，信息 %3 项。")
        .arg(result.errorCount).arg(result.warningCount).arg(result.infoCount));
    return result;
}

void hnProjectDiagnosticService::checkDatabaseTables(hnDBSqlite* database, ProjectDiagnosticResult& result)
{
    const char* databasePathValue = database->getDBPath();
    const QString databasePath = databasePathValue == nullptr ? QString() : QString::fromLocal8Bit(databasePathValue);
    if (databasePath.isEmpty())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_PATH_INVALID"),
            QStringLiteral("工程数据库路径为空，无法进行只读表结构检查。"));
        return;
    }

    sqlite3* readOnlyDatabase = nullptr;
    const QByteArray utf8DatabasePath = databasePath.toUtf8();
    if (sqlite3_open_v2(utf8DatabasePath.constData(), &readOnlyDatabase, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_READONLY_OPEN_FAILED"),
            QStringLiteral("工程数据库无法以只读方式打开，无法检查关键数据表。"), databasePath);
        if (readOnlyDatabase != nullptr)
        {
            sqlite3_close(readOnlyDatabase);
        }
        return;
    }

    QStringList requiredTables;
    requiredTables.append(QString::fromLatin1(MILE_INFO_TABLE));
    requiredTables.append(QString::fromLatin1(MARK_INFO_TABLE));
    requiredTables.append(QString::fromLatin1(MILEAGE_PILE_TABLE));
    hnRoadDiseaseTable* diseaseTable = database->getDiseaseTable();
    if (diseaseTable == nullptr)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_DISEASE_TABLE_INVALID"),
            QStringLiteral("工程数据库中的病害表服务不可用。"), databasePath);
    }
    else
    {
        const std::vector<std::string> diseaseTableNames = diseaseTable->GetAllDiseaseTableNames();
        for (size_t tableIndex = 0; tableIndex < diseaseTableNames.size(); ++tableIndex)
        {
            requiredTables.append(QString::fromLocal8Bit(diseaseTableNames.at(tableIndex).c_str()));
        }
    }

    for (int tableIndex = 0; tableIndex < requiredTables.size(); ++tableIndex)
    {
        const QByteArray tableName = requiredTables.at(tableIndex).toUtf8();
        sqlite3_stmt* statement = nullptr;
        const int prepareResult = sqlite3_prepare_v2(readOnlyDatabase,
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;", -1, &statement, nullptr);
        bool tableExists = false;
        if (prepareResult == SQLITE_OK && sqlite3_bind_text(statement, 1, tableName.constData(), -1, SQLITE_TRANSIENT) == SQLITE_OK)
        {
            tableExists = sqlite3_step(statement) == SQLITE_ROW;
        }
        if (statement != nullptr)
        {
            sqlite3_finalize(statement);
        }
        if (!tableExists)
        {
            addIssue(result, ProjectDiagnosticError, QStringLiteral("DATABASE_TABLE_MISSING"),
                QStringLiteral("工程数据库缺少关键数据表或该表无法只读查询：%1。").arg(requiredTables.at(tableIndex)), databasePath);
        }
        else
        {
            addIssue(result, ProjectDiagnosticInfo, QStringLiteral("DATABASE_TABLE_PRESENT"),
                QStringLiteral("关键数据表存在：%1（仅检查表结构存在，不代表表内数据完整）。").arg(requiredTables.at(tableIndex)), databasePath);
        }
    }
    sqlite3_close(readOnlyDatabase);
}

bool hnProjectDiagnosticService::writeReport(
    const ProjectDiagnosticResult& result,
    const QString& outputDirectory,
    QString& reportPath)
{
    QDir outputDir(outputDirectory);
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral(".")))
    {
        return false;
    }

    reportPath = outputDir.filePath(QStringLiteral("检查报告.txt"));
    QFile reportFile(reportPath);
    if (!reportFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return false;
    }

    QTextStream stream(&reportFile);
    stream.setCodec("UTF-8");
    stream << QStringLiteral("工程检查报告\n");
    stream << QStringLiteral("生成时间: ") << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) << QStringLiteral("\n");
    stream << QStringLiteral("错误: ") << result.errorCount << QStringLiteral("  警告: ") << result.warningCount
           << QStringLiteral("  信息: ") << result.infoCount << QStringLiteral("\n\n");

    for (int issueIndex = 0; issueIndex < result.issues.size(); ++issueIndex)
    {
        const ProjectDiagnosticIssue& issue = result.issues.at(issueIndex);
        QString severityText = QStringLiteral("信息");
        if (issue.severity == ProjectDiagnosticWarning)
        {
            severityText = QStringLiteral("警告");
        }
        else if (issue.severity == ProjectDiagnosticError)
        {
            severityText = QStringLiteral("错误");
        }

        stream << QStringLiteral("[") << severityText << QStringLiteral("][") << issue.code << QStringLiteral("] ")
               << issue.message << QStringLiteral("\n");
        if (!issue.path.isEmpty())
        {
            stream << QStringLiteral("路径: ") << issue.path << QStringLiteral("\n");
        }
    }

    return true;
}

bool hnProjectDiagnosticService::writeManifest(
    const ProjectDiagnosticResult& result,
    const QString& outputDirectory,
    QString& manifestPath)
{
    QDir outputDir(outputDirectory);
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral(".")))
    {
        return false;
    }

    manifestPath = outputDir.filePath(QStringLiteral("导出清单.tsv"));
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return false;
    }

    QTextStream stream(&manifestFile);
    stream.setCodec("UTF-8");
    stream << QStringLiteral("严重级别\t代码\t说明\t路径\n");
    for (int issueIndex = 0; issueIndex < result.issues.size(); ++issueIndex)
    {
        const ProjectDiagnosticIssue& issue = result.issues.at(issueIndex);
        stream << issue.severity << QStringLiteral("\t") << issue.code << QStringLiteral("\t")
               << issue.message << QStringLiteral("\t") << issue.path << QStringLiteral("\n");
    }
    return true;
}

bool hnProjectDiagnosticService::exportDiagnosticPackage(
    hnPro::hnProject* project,
    const QString& targetDirectory,
    ProjectDiagnosticResult& result,
    QString& packagePath,
    bool includeRawRutData)
{
    result = checkProject(project);
    packagePath.clear();
    if (project == nullptr || targetDirectory.isEmpty())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_ARGUMENT_INVALID"),
            QStringLiteral("诊断包输出目录或当前工程无效。"));
        return false;
    }

    QDir targetDir(targetDirectory);
    if (!targetDir.exists())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_ROOT_MISSING"),
            QStringLiteral("选择的诊断包输出目录不存在。"), targetDirectory);
        return false;
    }

    const QString finalDirectory = buildPackageDirectory(targetDirectory, project->getProjectName());
    const QString temporaryDirectory = finalDirectory + QStringLiteral(".building");
    if (QDir(temporaryDirectory).exists() || QFileInfo(finalDirectory).exists())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_TARGET_EXISTS"),
            QStringLiteral("诊断包目标已存在，已取消导出以避免覆盖已有文件。"), finalDirectory);
        return false;
    }

    QDir temporaryDir(temporaryDirectory);
    if (!temporaryDir.mkpath(QStringLiteral(".")))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_DIRECTORY_CREATE_FAILED"),
            QStringLiteral("无法创建诊断包临时目录。"), temporaryDirectory);
        return false;
    }

    const int checkErrorCount = result.errorCount;
    const QString sourceRoot = project->getAbsulotelyPath();
    copyFile(QDir(sourceRoot).filePath(QStringLiteral("ProjectInfo.xml")),
        temporaryDir.filePath(QStringLiteral("ProjectInfo.xml")), result, false);

    const hnCommon::PROJECT_TYPE projectType = project->getProjectType();
    if (projectType == hnCommon::PROJECT_2D_TYPE || projectType == hnCommon::PROJECT_23D_TYPE)
    {
        const QString source2DPath = project->get2DProPath();
        const QString destination2DPath = temporaryDir.filePath(project->get2DProName());
        const QStringList required2DFiles = QStringList()
            << QStringLiteral("Setting.ini") << QStringLiteral("ProjectInfo.txt")
            << QStringLiteral("Dmi2Mile.txt") << QStringLiteral("MileStoneCaliInfo.txt")
            << QStringLiteral("GPS2Mile.txt") << QStringLiteral("RoadTypeInfo.txt")
            << QStringLiteral("2d3dDiffSetting.ini");
        for (int fileIndex = 0; fileIndex < required2DFiles.size(); ++fileIndex)
        {
            const QString relativePath = required2DFiles.at(fileIndex);
            copyFile(QDir(source2DPath).filePath(relativePath), QDir(destination2DPath).filePath(relativePath), result, false);
        }

        const QStringList mappingFiles = QStringList()
            << QStringLiteral("RoadImg/Camera0/Road2Mile.txt")
            << QStringLiteral("RoadImg/SYN/gps.txt") << QStringLiteral("RoadImg/SYN/trigger.txt")
            << QStringLiteral("StreetImg/Camera0/Street2Mile.txt")
            << QStringLiteral("StreetImg/Camera1/Street2Mile.txt")
            << QStringLiteral("StreetImg/SYN/gps.txt") << QStringLiteral("StreetImg/SYN/trigger.txt")
            << QStringLiteral("RUT/camera0/orirut.txt") << QStringLiteral("RUT/camera1/orirut.txt")
            << QStringLiteral("IRIMTD/DAQ0/IRI_10m.txt") << QStringLiteral("IRIMTD/DAQ1/IRI_10m.txt");
        for (int fileIndex = 0; fileIndex < mappingFiles.size(); ++fileIndex)
        {
            const QString relativePath = mappingFiles.at(fileIndex);
            copyFile(QDir(source2DPath).filePath(relativePath), QDir(destination2DPath).filePath(relativePath), result, false);
        }

        if (includeRawRutData)
        {
            copyDirectory(
                QDir(source2DPath).filePath(QStringLiteral("camera0")),
                QDir(destination2DPath).filePath(QStringLiteral("camera0")),
                result,
                true);
        }

        hnPro::hn2DProject* project2D = project->get2DProject();
        if (project2D != nullptr)
        {
            if (project2D->_IsRoad)
            {
                copyImageSamples(QStringLiteral("路面"), project2D->getRoadPicturePath(),
                    temporaryDir.filePath(QStringLiteral("诊断样图/路面")), result);
            }
            if (project2D->_IsStreet)
            {
                copyImageSamples(QStringLiteral("左景观"), project2D->getLeftStreetPicturePath(),
                    temporaryDir.filePath(QStringLiteral("诊断样图/左景观")), result);
            }
            if (project2D->_IsDStreet)
            {
                copyImageSamples(QStringLiteral("右景观"), project2D->getRightStreetPicturePath(),
                    temporaryDir.filePath(QStringLiteral("诊断样图/右景观")), result);
            }
        }
    }

    if (projectType == hnCommon::PROJECT_XD_3D_TYPE ||
        projectType == hnCommon::PROJECT_JD_3D_TYPE ||
        projectType == hnCommon::PROJECT_23D_TYPE)
    {
        const QString source3DRoot = QDir(project->get3DProPath()).filePath(project->get3DProName());
        const QString destination3DRoot = temporaryDir.filePath(project->get3DProName());
        const QStringList required3DFiles = QStringList()
            << QStringLiteral("PointCloud/1/Mms-Cam-1.cam")
            << QStringLiteral("PointCloud/1/iScan-Cam-1.cam")
            << QStringLiteral("Image/pavement-cam-1.idx")
            << QStringLiteral("3dProjectConfig.ini") << QStringLiteral("mirroredSetting.ini")
            << QStringLiteral("Mms-Para.config") << QStringLiteral("Mms-Para.db");
        for (int fileIndex = 0; fileIndex < required3DFiles.size(); ++fileIndex)
        {
            const QString relativePath = required3DFiles.at(fileIndex);
            copyFile(QDir(source3DRoot).filePath(relativePath), QDir(destination3DRoot).filePath(relativePath), result, false);
        }
    }

    copyDirectory(QDir(sourceRoot).filePath(QStringLiteral("成果数据")),
        temporaryDir.filePath(QStringLiteral("成果数据")), result, false);
    copyDirectory(QDir(sourceRoot).filePath(QStringLiteral("诊断输出")),
        temporaryDir.filePath(QStringLiteral("诊断输出")), result, false);
    copyFile(logMgr::instance()->getFileName(),
        temporaryDir.filePath(QStringLiteral("运行日志.log")), result, false);

    QString reportPath;
    if (!writeReport(result, temporaryDir.filePath(QStringLiteral("诊断输出")), reportPath))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("REPORT_WRITE_FAILED"),
            QStringLiteral("无法在诊断包中写入检查报告。"));
    }

    QString manifestPath;
    if (!writeManifest(result, temporaryDir.filePath(QStringLiteral("诊断输出")), manifestPath))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("MANIFEST_WRITE_FAILED"),
            QStringLiteral("无法在诊断包中写入导出清单。"));
    }

    const QFileInfo finalInfo(finalDirectory);
    QDir parentDir(finalInfo.absolutePath());
    if (result.errorCount > checkErrorCount ||
        !parentDir.rename(QFileInfo(temporaryDirectory).fileName(), finalInfo.fileName()))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_NOT_COMMITTED"),
            QStringLiteral("诊断包未完成提交，临时目录已保留供排查。"), temporaryDirectory);
        return false;
    }

    packagePath = finalDirectory;
    addIssue(result, ProjectDiagnosticInfo, QStringLiteral("EXPORT_FINISHED"),
        QStringLiteral("诊断包导出完成。"), packagePath);
    return true;
}

bool hnProjectDiagnosticService::hasRawRutData(
    hnPro::hnProject* project,
    QString& cameraDirectory) const
{
    cameraDirectory.clear();
    if (project == nullptr)
    {
        return false;
    }

    const hnCommon::PROJECT_TYPE projectType = project->getProjectType();
    if (projectType != hnCommon::PROJECT_2D_TYPE &&
        projectType != hnCommon::PROJECT_23D_TYPE)
    {
        return false;
    }

    const QString candidateCameraDirectory =
        QDir(project->get2DProPath()).filePath(QStringLiteral("camera0"));
    const QString rawDataDirectory =
        QDir(candidateCameraDirectory).filePath(QStringLiteral("data"));
    QDirIterator fileIterator(
        rawDataDirectory,
        QDir::Files | QDir::NoSymLinks,
        QDirIterator::Subdirectories);
    if (!fileIterator.hasNext())
    {
        return false;
    }

    cameraDirectory = candidateCameraDirectory;
    return true;
}

void hnProjectDiagnosticService::addIssue(
    ProjectDiagnosticResult& result,
    ProjectDiagnosticSeverity severity,
    const QString& code,
    const QString& message,
    const QString& path)
{
    ProjectDiagnosticIssue issue;
    issue.severity = severity;
    issue.code = code;
    issue.message = message;
    issue.path = path;
    result.issues.append(issue);

    if (severity == ProjectDiagnosticError)
    {
        ++result.errorCount;
    }
    else if (severity == ProjectDiagnosticWarning)
    {
        ++result.warningCount;
    }
    else
    {
        ++result.infoCount;
    }
}

void hnProjectDiagnosticService::checkRequiredFile(
    ProjectDiagnosticResult& result,
    const QString& filePath,
    const QString& code,
    const QString& description,
    bool required)
{
    if (QFileInfo(filePath).isFile())
    {
        addIssue(result, ProjectDiagnosticInfo, QStringLiteral("FILE_PRESENT"),
            description + QStringLiteral("存在（仅检查文件是否存在）。"), filePath);
        return;
    }

    addIssue(result, required ? ProjectDiagnosticError : ProjectDiagnosticWarning, code,
        description + QStringLiteral("不存在。"), filePath);
}

void hnProjectDiagnosticService::checkImageChannel(
    ProjectDiagnosticResult& result,
    const QString& channelName,
    const QStringList& imagePaths,
    int interval,
    double expectedLength)
{
    // 不以 xxx2Mile 映射文件是否存在作为工程有效性判断条件。
    if (interval <= 0)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("IMAGE_INTERVAL_INVALID"),
            channelName + QStringLiteral("采集间隔必须大于零。"));
        return;
    }
    if (imagePaths.isEmpty())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("IMAGE_EMPTY"),
            channelName + QStringLiteral("没有可用图像。"));
        return;
    }

    if (!QFileInfo(imagePaths.first()).isFile() || !QFileInfo(imagePaths.last()).isFile())
    {
        addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_ENDPOINT_MISSING"),
            channelName + QStringLiteral("首张或末张图像不存在。"));
    }

    const int expectedCount = qRound(qAbs(expectedLength) / interval);
    const int toleranceCount = 5;
    if (expectedCount > toleranceCount && qAbs(imagePaths.size() - expectedCount) > toleranceCount)
    {
        addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_COUNT_ABNORMAL"),
            QStringLiteral("%1图像数量为 %2，按采集间隔 %3 米估算应约为 %4 张。")
            .arg(channelName).arg(imagePaths.size()).arg(interval).arg(expectedCount));
    }
}

void hnProjectDiagnosticService::check2DProject(
    hnPro::hnProject* project,
    ProjectDiagnosticResult& result)
{
    const QString project2DPath = project->get2DProPath();
    if (!QDir(project2DPath).exists())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("2D_DIRECTORY_MISSING"),
            QStringLiteral("二维工程目录不存在。"), project2DPath);
        return;
    }

    checkRequiredFile(result, QDir(project2DPath).filePath(QStringLiteral("Setting.ini")),
        QStringLiteral("SETTING_MISSING"), QStringLiteral("工程配置文件 Setting.ini"), true);
    const QString projectInfoPath = QDir(project2DPath).filePath(QStringLiteral("ProjectInfo.txt"));
    checkRequiredFile(result, projectInfoPath,
        QStringLiteral("PROJECT_INFO_MISSING"), QStringLiteral("工程信息文件 ProjectInfo.txt"), true);
    const hnCommon::hnProjectSetInfo setInfo = project->getCurProSetInfo();

    // ProjectInfo.txt 是采集工程方向的源数据；只读比较它与成果数据库中的运行参数，避免静默方向冲突。
    int projectInfoLineType = 0;
    QFile projectInfoFile(projectInfoPath);
    if (projectInfoFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream projectInfoStream(&projectInfoFile);
        projectInfoStream.setCodec("UTF-8");
        while (!projectInfoStream.atEnd())
        {
            const QString projectInfoLine = projectInfoStream.readLine();
            if (!projectInfoLine.contains(QStringLiteral("行车方向")))
            {
                continue;
            }
            if (projectInfoLine.contains(QStringLiteral("上行")))
            {
                projectInfoLineType = 1;
            }
            else if (projectInfoLine.contains(QStringLiteral("下行")))
            {
                projectInfoLineType = -1;
            }
            break;
        }
        projectInfoFile.close();
    }

    bool hasDetailedDirectionIssue = false;
    if ((projectInfoLineType == 1 || projectInfoLineType == -1) &&
        setInfo.nLineType != projectInfoLineType)
    {
        const QString projectInfoDirection = projectInfoLineType == 1
            ? QStringLiteral("上行") : QStringLiteral("下行");
        const QString databaseDirection = setInfo.nLineType == 1
            ? QStringLiteral("上行") : (setInfo.nLineType == -1 ? QStringLiteral("下行") : QStringLiteral("无效值"));
        addIssue(result, ProjectDiagnosticError, QStringLiteral("PROJECT_DATABASE_DIRECTION_MISMATCH"),
            QStringLiteral("工程方向不一致：ProjectInfo.txt 为%1（%2），成果数据库 SETTING_INFO.LineType=%3（%4）。"
                "软件会按错误方向生成路面图像里程序列，可能在第一张图像处提前结束，导致路面图像视图为空。")
            .arg(projectInfoDirection).arg(projectInfoLineType)
            .arg(setInfo.nLineType).arg(databaseDirection),
            projectInfoPath);
        hasDetailedDirectionIssue = true;
    }

    const bool isForward = setInfo.nLineType == 1;
    if (!hasDetailedDirectionIssue &&
        ((isForward && setInfo.dBegMile >= setInfo.dEndMile) ||
        (!isForward && setInfo.dBegMile <= setInfo.dEndMile)))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("MILE_DIRECTION_INVALID"),
            QStringLiteral("起止桩号与行车方向不一致：SETTING_INFO.LineType=%1，起点桩号 %2，终点桩号 %3。"
                "该错误可能导致路面图像里程序列为空，软件中看不到路面图像。")
            .arg(setInfo.nLineType)
            .arg(setInfo.dBegMile, 0, 'f', 3)
            .arg(setInfo.dEndMile, 0, 'f', 3));
    }

    hnPro::hn2DProject* project2D = project->get2DProject();
    if (project2D == nullptr)
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("2D_OBJECT_MISSING"),
            QStringLiteral("二维工程数据未成功加载。"));
        return;
    }

    const double expectedLength = project->getProjectType() == hnCommon::PROJECT_2D_TYPE
        ? setInfo.dLength : setInfo.dEndEnclMile;
    if (project2D->_IsRoad)
    {
        checkImageChannel(result, QStringLiteral("路面"),
            project2D->getRoadPicturePath(), project2D->_RoadImgDis, expectedLength);
        checkImageSequenceChannel(result, project2DPath, QStringLiteral("RoadImg/Camera0"));
    }
    if (project2D->_IsStreet)
    {
        checkImageChannel(result, QStringLiteral("左景观"),
            project2D->getLeftStreetPicturePath(), project2D->_StreetImgDis, expectedLength);
        checkImageSequenceChannel(result, project2DPath, QStringLiteral("StreetImg/Camera0"));
    }
    if (project2D->_IsDStreet)
    {
        checkImageChannel(result, QStringLiteral("右景观"),
            project2D->getRightStreetPicturePath(), project2D->_StreeRightImgDis, expectedLength);
        checkImageSequenceChannel(result, project2DPath, QStringLiteral("StreetImg/Camera1"));
    }
    if (project2D->_IsRut)
    {
        checkRequiredFile(result, QDir(project2DPath).filePath(QStringLiteral("RUT/camera0/orirut.txt")),
            QStringLiteral("RUT_DATA_MISSING"), QStringLiteral("车辙数据文件"), false);
    }
    if (project2D->_IsIRIMTD)
    {
        checkRequiredFile(result, QDir(project2DPath).filePath(QStringLiteral("IRIMTD/DAQ0/IRI_10m.txt")),
            QStringLiteral("IRI_DATA_MISSING"), QStringLiteral("IRI 数据文件"), false);
    }
}

void hnProjectDiagnosticService::checkImageSequenceChannel(
    ProjectDiagnosticResult& result,
    const QString& project2DPath,
    const QString& channelPath)
{
    const QDir project2DDir(project2DPath);
    const QString channelDirectoryPath = project2DDir.filePath(channelPath);
    const QFileInfo channelDirectoryInfo(channelDirectoryPath);
    if (!channelDirectoryInfo.exists() || !channelDirectoryInfo.isDir())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("IMAGE_CHANNEL_DIRECTORY_MISSING"),
            QStringLiteral("图像通道目录缺失：%1。").arg(channelPath), channelPath);
        return;
    }
    if (!channelDirectoryInfo.isReadable())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("IMAGE_CHANNEL_DIRECTORY_UNREADABLE"),
            QStringLiteral("图像通道目录无法读取：%1。").arg(channelPath), channelPath);
        return;
    }

    const QRegExp chunkNamePattern(QStringLiteral("^Image_(\\d+)$"));
    const QFileInfoList childDirectories = QDir(channelDirectoryPath).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QList<ImageChunkDirectory> chunks;
    for (int directoryIndex = 0; directoryIndex < childDirectories.size(); ++directoryIndex)
    {
        const QFileInfo& directoryInfo = childDirectories.at(directoryIndex);
        const QString directoryName = directoryInfo.fileName();
        if (directoryName.startsWith(QStringLiteral("Image_")) && !chunkNamePattern.exactMatch(directoryName))
        {
            addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_DIRECTORY_MALFORMED"),
                QStringLiteral("图像分段目录格式不正确：%1。").arg(directoryName),
                QDir(channelPath).filePath(directoryName));
            continue;
        }
        if (chunkNamePattern.exactMatch(directoryName))
        {
            bool isIndexValid = false;
            const int chunkNumber = chunkNamePattern.cap(1).toInt(&isIndexValid);
            if (!isIndexValid)
            {
                addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_DIRECTORY_MALFORMED"),
                    QStringLiteral("图像分段目录编号无效或超出整数范围：%1。").arg(directoryName),
                    QDir(channelPath).filePath(directoryName));
                continue;
            }
            ImageChunkDirectory chunk;
            chunk.index = chunkNumber;
            chunk.info = directoryInfo;
            chunks.append(chunk);
        }
    }
    qSort(chunks.begin(), chunks.end(), imageChunkDirectoryLessThan);

    if (chunks.isEmpty() || chunks.first().index != 0)
    {
        addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_DIRECTORY_MISSING"),
            QStringLiteral("图像通道 %1 中缺少 Image_0000。").arg(channelPath),
            QDir(channelPath).filePath(QStringLiteral("Image_0000")));
    }

    for (int chunkIndex = 1; chunkIndex < chunks.size(); ++chunkIndex)
    {
        const int previousIndex = chunks.at(chunkIndex - 1).index;
        const int currentIndex = chunks.at(chunkIndex).index;
        for (int missingIndex = previousIndex + 1; missingIndex < currentIndex; ++missingIndex)
        {
            const QString missingName = QStringLiteral("Image_%1").arg(missingIndex);
            addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_DIRECTORY_GAP"),
                QStringLiteral("缺少 %1，位于 %2 与 %3 之间。")
                .arg(missingName, chunks.at(chunkIndex - 1).info.fileName(), chunks.at(chunkIndex).info.fileName()),
                QDir(channelPath).filePath(missingName));
        }
    }

    QList<int> imageCounts;
    const QRegExp serialNamePattern(QStringLiteral("^(\\d+)_"));
    const QStringList imageNameFilters = QStringList()
        << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg")
        << QStringLiteral("*.JPG") << QStringLiteral("*.JPEG");
    for (int chunkIndex = 0; chunkIndex < chunks.size(); ++chunkIndex)
    {
        const QFileInfo& chunkInfo = chunks.at(chunkIndex).info;
        const QString relativeChunkPath = project2DDir.relativeFilePath(chunkInfo.absoluteFilePath());
        const QFileInfoList imageFiles = QDir(chunkInfo.absoluteFilePath()).entryInfoList(
            imageNameFilters, QDir::Files, QDir::Name);
        imageCounts.append(imageFiles.size());
        if (imageFiles.isEmpty())
        {
            addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_EMPTY"),
                QStringLiteral("图像分段中没有 JPG/JPEG 文件：%1。").arg(relativeChunkPath),
                relativeChunkPath);
        }

        QMap<int, QStringList> serialFiles;
        for (int imageIndex = 0; imageIndex < imageFiles.size(); ++imageIndex)
        {
            const QString fileName = imageFiles.at(imageIndex).fileName();
            if (serialNamePattern.indexIn(fileName) != 0)
            {
                addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_INVALID"),
                    QStringLiteral("图像文件缺少序号前缀：目录 %1，文件 %2。").arg(relativeChunkPath, fileName),
                    relativeChunkPath);
                continue;
            }
            bool isSerialValid = false;
            const int serial = serialNamePattern.cap(1).toInt(&isSerialValid);
            if (!isSerialValid)
            {
                addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_INVALID"),
                    QStringLiteral("图像文件序号无效或超出整数范围：目录 %1，文件 %2。")
                    .arg(relativeChunkPath, fileName), relativeChunkPath);
                continue;
            }
            serialFiles[serial].append(fileName);
        }

        QMap<int, QStringList>::const_iterator serialIt = serialFiles.constBegin();
        while (serialIt != serialFiles.constEnd())
        {
            if (serialIt.value().size() > 1)
            {
                addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_DUPLICATE"),
                    QStringLiteral("图像序号重复：目录 %2 中序号 %1 对应文件 %3。")
                    .arg(serialIt.key()).arg(relativeChunkPath, serialIt.value().join(QStringLiteral(", "))),
                    relativeChunkPath);
            }
            ++serialIt;
        }

        if (!serialFiles.isEmpty() && chunkIndex + 1 == chunks.size())
        {
            int previousSerial = serialFiles.constBegin().key();
            for (int missingSerial = 0; missingSerial < previousSerial; ++missingSerial)
            {
                addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_MISSING"),
                    QStringLiteral("图像序号缺失：目录 %2 开头、%3 之前缺少序号 %1。")
                    .arg(missingSerial).arg(relativeChunkPath, serialFiles.value(previousSerial).first()),
                    relativeChunkPath);
            }
            QMap<int, QStringList>::const_iterator nextSerialIt = serialFiles.constBegin();
            ++nextSerialIt;
            while (nextSerialIt != serialFiles.constEnd())
            {
                for (int missingSerial = previousSerial + 1; missingSerial < nextSerialIt.key(); ++missingSerial)
                {
                    addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_MISSING"),
                        QStringLiteral("图像序号缺失：目录 %2 中缺少序号 %1，前后图片为 %3 与 %4。")
                        .arg(missingSerial).arg(relativeChunkPath,
                            serialFiles.value(previousSerial).first(), nextSerialIt.value().first()),
                        relativeChunkPath);
                }
                previousSerial = nextSerialIt.key();
                ++nextSerialIt;
            }
        }

        if (chunkIndex + 1 < chunks.size())
        {
            for (int requiredSerial = 0; requiredSerial < kImageChunkFrameCount; ++requiredSerial)
            {
                if (!serialFiles.contains(requiredSerial))
                {
                    addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_SERIAL_MISSING"),
                        QStringLiteral("非最后图像分段 %2 中缺少必需序号 %1。")
                        .arg(requiredSerial).arg(relativeChunkPath), relativeChunkPath);
                }
            }
        }
    }

    for (int chunkIndex = 0; chunkIndex + 1 < chunks.size(); ++chunkIndex)
    {
        if (imageCounts.at(chunkIndex) != kImageChunkFrameCount)
        {
            const QString relativeChunkPath = project2DDir.relativeFilePath(chunks.at(chunkIndex).info.absoluteFilePath());
            addIssue(result, ProjectDiagnosticWarning, QStringLiteral("IMAGE_CHUNK_COUNT_ABNORMAL"),
                QStringLiteral("非最后图像分段 %1 中有 %2 个 JPG/JPEG 文件，应为 %3 个。")
                .arg(relativeChunkPath).arg(imageCounts.at(chunkIndex)).arg(kImageChunkFrameCount),
                relativeChunkPath);
        }
    }
}

void hnProjectDiagnosticService::check3DProject(
    hnPro::hnProject* project,
    ProjectDiagnosticResult& result)
{
    const QString project3DRoot = QDir(project->get3DProPath()).filePath(project->get3DProName());
    if (!QDir(project3DRoot).exists())
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("3D_DIRECTORY_MISSING"),
            QStringLiteral("三维工程目录不存在。"), project3DRoot);
        return;
    }

    checkRequiredFile(result, QDir(project3DRoot).filePath(QStringLiteral("Mms-Para.db")),
        QStringLiteral("3D_PARAMETER_DB_MISSING"), QStringLiteral("三维参数数据库 Mms-Para.db"), true);
    checkRequiredFile(result, QDir(project3DRoot).filePath(QStringLiteral("3dProjectConfig.ini")),
        QStringLiteral("3D_SETTING_MISSING"), QStringLiteral("三维工程配置文件"), false);
    checkRequiredFile(result, QDir(project3DRoot).filePath(QStringLiteral("PointCloud/1/Mms-Cam-1.cam")),
        QStringLiteral("3D_CAMERA_MISSING"), QStringLiteral("三维相机文件 Mms-Cam-1.cam"), false);
    checkRequiredFile(result, QDir(project3DRoot).filePath(QStringLiteral("PointCloud/1/iScan-Cam-1.cam")),
        QStringLiteral("3D_CAMERA_MISSING"), QStringLiteral("三维相机文件 iScan-Cam-1.cam"), false);
}

bool hnProjectDiagnosticService::copyFile(
    const QString& sourcePath,
    const QString& destinationPath,
    ProjectDiagnosticResult& result,
    bool required)
{
    if (!QFileInfo(sourcePath).isFile())
    {
        addIssue(result, required ? ProjectDiagnosticError : ProjectDiagnosticWarning,
            QStringLiteral("EXPORT_SOURCE_FILE_MISSING"), QStringLiteral("诊断包源文件不存在，未复制。"), sourcePath);
        return !required;
    }

    QDir destinationDir(QFileInfo(destinationPath).absolutePath());
    if (!destinationDir.exists() && !destinationDir.mkpath(QStringLiteral(".")))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_DIRECTORY_CREATE_FAILED"),
            QStringLiteral("无法创建诊断包目标目录。"), destinationDir.absolutePath());
        return false;
    }
    if (!QFile::copy(sourcePath, destinationPath))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_FILE_COPY_FAILED"),
            QStringLiteral("无法复制诊断包文件。"), sourcePath);
        return false;
    }
    addIssue(result, ProjectDiagnosticInfo, QStringLiteral("EXPORTED_FILE"),
        QStringLiteral("已复制诊断包文件。"), destinationPath);
    return true;
}

bool hnProjectDiagnosticService::copyDirectory(
    const QString& sourcePath,
    const QString& destinationPath,
    ProjectDiagnosticResult& result,
    bool required)
{
    QDir sourceDir(sourcePath);
    if (!sourceDir.exists())
    {
        addIssue(result, required ? ProjectDiagnosticError : ProjectDiagnosticWarning,
            QStringLiteral("EXPORT_SOURCE_DIRECTORY_MISSING"), QStringLiteral("诊断包源目录不存在，未复制。"), sourcePath);
        return !required;
    }

    QDir destinationDir(destinationPath);
    if (!destinationDir.exists() && !destinationDir.mkpath(QStringLiteral(".")))
    {
        addIssue(result, ProjectDiagnosticError, QStringLiteral("EXPORT_DIRECTORY_CREATE_FAILED"),
            QStringLiteral("无法创建诊断包目录。"), destinationPath);
        return false;
    }

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    bool isSuccessful = true;
    for (int entryIndex = 0; entryIndex < entries.size(); ++entryIndex)
    {
        const QFileInfo entry = entries.at(entryIndex);
        const QString destinationEntryPath = destinationDir.filePath(entry.fileName());
        if (entry.isDir())
        {
            isSuccessful = copyDirectory(entry.absoluteFilePath(), destinationEntryPath, result, true) && isSuccessful;
        }
        else
        {
            isSuccessful = copyFile(entry.absoluteFilePath(), destinationEntryPath, result, true) && isSuccessful;
        }
    }
    return isSuccessful;
}

void hnProjectDiagnosticService::copyImageSamples(
    const QString& channelName,
    const QStringList& imagePaths,
    const QString& destinationDirectory,
    ProjectDiagnosticResult& result)
{
    if (imagePaths.isEmpty())
    {
        return;
    }

    const int sampleCount = qMin(30, imagePaths.size());
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        const int imageIndex = sampleCount == 1 ? 0 :
            qRound(static_cast<double>(sampleIndex) * (imagePaths.size() - 1) / (sampleCount - 1));
        const QString sourcePath = imagePaths.at(imageIndex);
        const QString destinationName = QStringLiteral("%1_%2_%3")
            .arg(channelName).arg(imageIndex, 6, 10, QLatin1Char('0')).arg(QFileInfo(sourcePath).fileName());
        copyFile(sourcePath, QDir(destinationDirectory).filePath(destinationName), result, false);
    }
}

QString hnProjectDiagnosticService::buildPackageDirectory(const QString& targetDirectory, const QString& projectName) const
{
    const QString safeProjectName = projectName.isEmpty() ? QStringLiteral("UnnamedProject") : projectName;
    return QDir(targetDirectory).filePath(safeProjectName + QStringLiteral("_诊断包_") +
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
}
