#include "hn2dDiseaseExchangeService.h"
#include "hnOutExcelMile.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnApplication/hnDiseaseService.h"
#include "../hnProject/hnProjectManager.h"
#include "xlsxdocument.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QTimer>
#include <QMessageBox>

// 仅接收已经复制到验证目录的工程，绝不对客户原件运行。
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTextStream out(stdout); out.setCodec("UTF-8");
    if (argc < 3) return 2;
    const QString root = QString::fromLocal8Bit(argv[1]);
    if (!root.contains(QStringLiteral("_codex_build_verify"))) return 3;
    QTimer watchdog;
    QObject::connect(&watchdog, &QTimer::timeout, []() {
        for (QWidget* widget : QApplication::topLevelWidgets())
            if (QMessageBox* message = qobject_cast<QMessageBox*>(widget))
            { QTextStream(stderr) << message->text() << "\n"; message->reject(); }
    });
    watchdog.start(100);
    HnXRSettings::getInstance()->SetConfigFilePath(QDir(root).filePath("test-settings.ini"));
    HnXRSettings::getInstance()->Init();
    hnApp::hnDataManager* data = hnApp::hnDataManager::getDataManager();
    if (!data->initRoadStandardInfo()) { out << "FAIL parameters\n"; return 4; }
    vector<hnProjectDataInfo> projects;
    PROJECT_TYPE type = PROJECT_2D_TYPE;
    if (!data->getAllProject(root, projects, type)) return 5;
    data->setProjectDiseaseVector(projects);
    if (!data->initProject(projects)) { out << "FAIL open\n"; return 5; }
    data->getProjectManager()->initAllProjectMileVector();
    hnPro::hnProject* project = data->getProjectManager()->getAllBaseProject().front();
    if (!project || !data->setCurrentProject(project->get2DProName())) return 6;
    hn2dDiseaseExchangeService service(project);
    const auto result = service.replaceDiseases();
    out << "IMPORT " << result.success << " " << result.roadCount << " " << result.streetCount << " " << result.error << "\n";
    if (!result.success) return 7;
    const auto info = project->getCurProSetInfo();
    out << "SET " << data->isOpenProject() << " " << project->getCurrentMileVector().size() << " " << info.dRoadWidth << " " << QString::fromLocal8Bit(info.strRoadStandard) << " " << info.nDrawType << " " << info.nRSurfaceType << "\n";
    for (const auto& mark : project->getCurrentMarkVector()) out << "MARK " << mark.nType << " " << mark.dEnclMile << " " << QString::fromLocal8Bit(mark.strMark) << "\n";
    data->getDiseaseService()->invalidateCache();
    QVector<hnRoadDiseaseInfo> diseases = data->getDiseaseService()->getAllRoadDiseases();
    out << "READ " << diseases.size() << "\n";
    if (diseases.size() != 31) return 11;
    // 从实际入库尺寸重新走手绘入口，确认没有按导入来源区分面积规则。
    for (const hnRoadDiseaseInfo& disease : diseases)
    {
        hnRoadDiseaseInfo manual = disease;
        data->setDiseaseCalcuteSize(manual);
        if (qAbs(manual.dArea - disease.dArea) > 1e-12) return 12;
    }
    out << "PASS shared drawing calculation\n";
    QXlsx::Document report;
    report.write(1,1,"Interval"); report.write(1,2,"Start"); report.write(1,3,"End"); report.write(1,4,"PCI");
    int row = 2;
    for (int interval : {10,100,1000})
    {
        for (int start = 0; start < 140; start += interval)
        {
            int end = qMin(140, start + interval);
            hnOutExcelMile mile(project, HnProjectEnums::CityRoad);
            mile.RoadSurface = static_cast<ROAD_SURFACE_TYPE>(0);
            mile.RoadSurfaceStr = QStringLiteral("沥青");
            mile.RoadDegreestr = QStringLiteral("主干路");
            mile.setStartMile(start); mile.setEndMile(end);
            mile.setStartDmi(start); mile.setEndDmi(end);
            mile.setSurveyWidth(3.75);
            if (!mile.StartCalculate(true) || !mile.calculateDrScore(3.75, diseases)) return 8;
            const QString pci = mile.getPCIExcelStr();
            out << "PCI " << interval << " " << start << " " << end << " " << pci << "\n";
            report.write(row,1,interval); report.write(row,2,start); report.write(row,3,end); report.write(row,4,pci.toDouble()); ++row;
        }
    }
    if (!report.saveAs(QString::fromLocal8Bit(argv[2]))) return 9;
    const auto repeat = service.replaceDiseases();
    data->getDiseaseService()->invalidateCache();
    if (!repeat.success || data->getDiseaseService()->getAllRoadDiseases().size() != diseases.size()) return 10;
    out << "PASS repeat\n";
    const QString dbPath = QString::fromLocal8Bit(project->getDB()->getDBPath());
    QFile snapshot(dbPath);
    if (!snapshot.open(QIODevice::ReadOnly)) return 13;
    const QByteArray beforeCancel = snapshot.readAll(); snapshot.close();
    int checks = 0;
    const auto stopped = service.replaceDiseases([&checks]() { return ++checks > 40; });
    out << "CANCEL " << stopped.stopped << " " << stopped.error << "\n";
    if (!snapshot.open(QIODevice::ReadOnly)) return 14;
    const QByteArray afterCancel = snapshot.readAll(); snapshot.close();
    if (!stopped.stopped || beforeCancel != afterCancel) return 15;
    out << "PASS cancellation restores database bytes\n";

    // 故意在写入中途触发 SQLite 失败，整工程旧病害必须保留。
    if (!project->getDB()->executeDB("CREATE TRIGGER test_import_abort BEFORE INSERT ON DisXL BEGIN SELECT RAISE(ABORT, 'test'); END")) return 16;
    if (!snapshot.open(QIODevice::ReadOnly)) return 17;
    const QByteArray beforeFailure = snapshot.readAll(); snapshot.close();
    const auto failed = service.replaceDiseases();
    if (!snapshot.open(QIODevice::ReadOnly)) return 18;
    const QByteArray afterFailure = snapshot.readAll(); snapshot.close();
    const bool preserved = !failed.success && beforeFailure == afterFailure;
    project->getDB()->executeDB("DROP TRIGGER test_import_abort");
    if (!preserved) return 19;
    out << "PASS write failure restores database bytes\n";
    const QString secondRoot = QDir(root).absoluteFilePath(QStringLiteral("../second/") + QFileInfo(root).fileName());
    vector<hnProjectDataInfo> secondInfos;
    if (!data->getAllProject(secondRoot, secondInfos, type)) return 20;
    data->setProjectDiseaseVector(secondInfos);
    hnPro::hnProject second;
    if (!second.openProject(secondInfos.front())) return 21;
    second.initMileList();
    second.setCurProject();
    if (!snapshot.open(QIODevice::ReadOnly)) return 22;
    const QByteArray firstBefore = snapshot.readAll(); snapshot.close();
    hn2dDiseaseExchangeService secondService(&second);
    const auto secondResult = secondService.replaceDiseases();
    if (!snapshot.open(QIODevice::ReadOnly)) return 23;
    const QByteArray firstAfter = snapshot.readAll(); snapshot.close();
    if (!secondResult.success || secondResult.roadCount != 31 || firstBefore != firstAfter || data->getCurrentProject() != project) return 24;
    out << "PASS explicit project isolation\n";
    // 混合规范和无法识别的文本应静默跳过，不影响其后的有效病害。
    const QString badPath = QDir(secondRoot).filePath(QStringLiteral("RoadImg/Camera0/Image_0000/000_094045633.jpg.txt"));
    QFile bad(badPath);
    if (!bad.open(QIODevice::ReadOnly)) return 25;
    const QByteArray source = bad.readAll(); bad.close();
    if (!bad.open(QIODevice::WriteOnly | QIODevice::Append)) return 26;
    bad.write("\ninvalid disease\n");
    bad.write(QStringLiteral("10 20 30 40 其他规范病害 0 沥青\n").toUtf8());
    bad.close();
    QFile secondDb(QString::fromLocal8Bit(second.getDB()->getDBPath()));
    secondDb.open(QIODevice::ReadOnly); const QByteArray secondBefore = secondDb.readAll(); secondDb.close();
    const auto invalid = secondService.replaceDiseases();
    secondDb.open(QIODevice::ReadOnly); const QByteArray secondAfter = secondDb.readAll(); secondDb.close();
    bad.open(QIODevice::WriteOnly | QIODevice::Truncate); bad.write(source); bad.close();
    if (!invalid.success || !invalid.error.isEmpty() || invalid.roadCount != 31) return 27;
    out << "PASS mixed standards and malformed road lines are silently skipped\n";
    if (!bad.open(QIODevice::WriteOnly | QIODevice::Append)) return 28;
    bad.write("\ninvalid disease\n");
    QByteArray valid = source.split('\n').first().trimmed();
    bad.write(valid + "\n"); bad.close();
    const auto continued = secondService.replaceDiseases();
    bad.open(QIODevice::WriteOnly | QIODevice::Truncate); bad.write(source); bad.close();
    if (!continued.success || continued.roadCount != 32 || !continued.error.isEmpty()) return 29;
    out << "PASS valid disease after unknown line still imported\n";
    out.flush();
    return 0;
}
