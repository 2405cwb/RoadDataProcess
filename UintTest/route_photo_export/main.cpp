#include "hnRoutePhotoExportService.h"
#include "hnRoutePhotoExportDialog.h"
#include "xlsxdocument.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QSettings>
#include <QSet>
#include <QFileInfo>

// 独立集成测试：只在临时目录造数据，真实工程始终只读。
class RoutePhotoTests : public hnRoutePhotoObserver
{
public:
    int failures = 0;
    bool stop = false;
    int cancelAt = -1;
    bool cancelled() const override { return stop; }
    void progress(int value, const QString&) override
    {
        if (cancelAt >= 0 && value >= cancelAt)
        {
            stop = true;
        }
    }
    void expect(bool condition, const QString& name)
    {
        QTextStream(stdout) << (condition ? "PASS " : "FAIL ") << name << "\n";
        if (!condition)
        {
            ++failures;
        }
    }
    void write(const QString& path, const QByteArray& bytes)
    {
        QFile file(path);
        expect(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), QStringLiteral("fixture write"));
    }
    hnRoutePhotoProject fixture(const QString& root)
    {
        hnRoutePhotoProject p;
        p.name = QStringLiteral("测试下行");
        p.path = root;
        p.roadCode = QStringLiteral("Y099511403");
        p.date = QDate(2026, 7, 28);
        p.startMile = 30;
        p.endMile = 0;
        QDir().mkpath(root + QStringLiteral("/StreetImg/Camera0/Image_0000"));
        write(root + QStringLiteral("/Setting.ini"), "[Distance]\nStreetDis=10\n");
        write(root + QStringLiteral("/GPS2Mile.txt"), "000000000 104.1234567 30.1234567 0 0 0\n000001000 104.1234568 30.1234568 0 10 10\n000002000 104.1234569 30.1234569 0 20 20\n000003000 104.1234570 30.1234570 0 30 30\n");
        const QStringList names = QStringList() << QStringLiteral("000_235958000.jpg") << QStringLiteral("001_235959000.jpg")
            << QStringLiteral("002_000000000.jpg") << QStringLiteral("003_000001000.jpg");
        QByteArray index;
        QImage image(2448, 2048, QImage::Format_RGB32);
        image.fill(QColor(100, 140, 160));
        for (int i = 0; i < 4; ++i)
        {
            image.save(root + QStringLiteral("/StreetImg/Camera0/Image_0000/") + names[i], "JPG");
            index += QByteArray::number(30 - 10 * i) + " \\Image_0000\\" + names[i].toUtf8() + "\n";
        }
        write(root + QStringLiteral("/StreetImg/Camera0/Street2Mile.txt"), index);
        return p;
    }
    void run(const QString& destination)
    {
        QTemporaryDir data;
        const hnRoutePhotoProject project = fixture(data.path() + QStringLiteral("/中文工程"));
        hnRoutePhotoExportService service;
        hnRoutePhotoCheck result = service.check(QVector<hnRoutePhotoProject>() << project, 2026);
        expect(result.errors.isEmpty() && result.photos.size() == 4, QStringLiteral("valid downhill fixture"));
        expect(result.photos.size() == 4 && result.photos[2].captured.date() == QDate(2026, 7, 29), QStringLiteral("midnight rollover"));
        hnRoutePhotoProject uphill = fixture(data.path() + QStringLiteral("/上行工程"));
        uphill.startMile = 0;
        uphill.endMile = 30;
        write(uphill.path + QStringLiteral("/StreetImg/Camera0/Street2Mile.txt"), "0 \\Image_0000\\000_235958000.jpg\n10 \\Image_0000\\001_235959000.jpg\n20 \\Image_0000\\002_000000000.jpg\n30 \\Image_0000\\003_000001000.jpg\n");
        result = service.check(QVector<hnRoutePhotoProject>() << project << uphill, 2026);
        expect(result.errors.isEmpty() && result.photos.size() == 8, QStringLiteral("batch same route both directions"));
        QString output, error;
        expect(service.exportPhotos(result, 2026, destination, output, error), QStringLiteral("export succeeds: ") + error);
        QTextStream(stdout) << "OUTPUT " << output << "\n";
        QXlsx::Document xlsx(output + QStringLiteral("/照片索引.xlsx"));
        expect(xlsx.read(1, 1).toString() == QStringLiteral("路线编码") && xlsx.read(1, 5).toString() == QStringLiteral("照片路径")
            && !xlsx.read(1, 6).isValid() && xlsx.read(9, 1).toString() == project.roadCode, QStringLiteral("five column eight row index"));
        QSet<QString> paths;
        for (int row = 2; row <= 9; ++row)
        {
            const QString relative = xlsx.read(row, 5).toString();
            const QString path = output + QLatin1Char('/') + relative;
            const QImage image(path);
            paths.insert(relative);
            expect(relative.startsWith(QStringLiteral("line/Y099511403/2026/")) && image.size() == QSize(2448, 2048)
                && QFileInfo(path).size() <= 2000000, QStringLiteral("relative path dimensions and bytes"));
        }
        expect(paths.size() == 8, QStringLiteral("unique GUID photos"));
        QImage painted(output + QLatin1Char('/') + xlsx.read(2, 5).toString());
        int whitePixels = 0;
        for (int y = painted.height() - 180; y < painted.height(); ++y)
        {
            for (int x = 0; x < 600; ++x)
            {
                const QColor pixel(painted.pixel(x, y));
                if (pixel.red() > 220 && pixel.green() > 220 && pixel.blue() > 220)
                {
                    ++whitePixels;
                }
            }
        }
        expect(whitePixels > 200, QStringLiteral("watermark text visibly rendered"));
        hnRoutePhotoExportDialog dialog(QVector<hnRoutePhotoProject>() << project << uphill);
        QMetaObject::invokeMethod(&dialog, "checkProjects", Qt::DirectConnection);
        hnRoutePhotoExportWorker* worker = dialog.findChild<hnRoutePhotoExportWorker*>();
        expect(worker && worker->wait(30000), QStringLiteral("dialog background check completes"));
        QApplication::processEvents();
        dialog.grab().save(destination + QStringLiteral("/dialog.png"));
        hnRoutePhotoExportWorker exporter;
        exporter.projects = QVector<hnRoutePhotoProject>() << project << uphill;
        exporter.exportRequested = true;
        exporter.destination = destination;
        exporter.start();
        expect(exporter.wait(30000) && exporter.error.isEmpty() && !exporter.outputPath.isEmpty(), QStringLiteral("background worker exports"));
        cancelAt = 0;
        expect(!service.exportPhotos(result, 2026, destination, output, error, this), QStringLiteral("cancel export"));
        stop = false;
        cancelAt = -1;
        expect(QDir(destination).entryList(QStringList() << QStringLiteral(".route-photo-*"), QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty(), QStringLiteral("cancel removes staging"));
        write(project.path + QStringLiteral("/Setting.ini"), "[Distance]\nStreetDis=5\n");
        QFile::remove(uphill.path + QStringLiteral("/GPS2Mile.txt"));
        result = service.check(QVector<hnRoutePhotoProject>() << project << uphill, 2026);
        const QString issues = result.errors.join(QStringLiteral("\n"));
        expect(issues.contains(QStringLiteral("StreetDis")) && issues.contains(QStringLiteral("GPS 桩号")), QStringLiteral("batch aggregates errors"));
        expect(!service.exportPhotos(result, 2026, destination, output, error), QStringLiteral("invalid batch blocked"));
        write(project.path + QStringLiteral("/Setting.ini"), "[Distance]\nStreetDis=10\n");
        write(project.path + QStringLiteral("/HighGps2Mile.txt"), "bad gps\n");
        result = service.check(QVector<hnRoutePhotoProject>() << project, 2026);
        expect(result.errors.isEmpty() && !result.warnings.isEmpty(), QStringLiteral("invalid high GPS falls back"));
        write(project.path + QStringLiteral("/StreetImg/Camera0/Street2Mile.txt"), "30 \\Image_0000\\000_235958000.jpg\n19 \\Image_0000\\001_235959000.jpg\n10 \\Image_0000\\002_000000000.jpg\n0 \\Image_0000\\003_000001000.jpg\n");
        result = service.check(QVector<hnRoutePhotoProject>() << project, 2026);
        expect(result.errors.join(QStringLiteral("\n")).contains(QStringLiteral("间隔异常")), QStringLiteral("11 meter gap blocked"));
        write(project.path + QStringLiteral("/GPS2Mile.txt"), "000000000 104 30 0 10 10\n000000000 104 30 0 20 20\n");
        result = service.check(QVector<hnRoutePhotoProject>() << project, 2026);
        expect(result.errors.join(QStringLiteral("\n")).contains(QStringLiteral("超出 GPS")), QStringLiteral("no GPS endpoint extrapolation"));
        QFile::remove(project.path + QStringLiteral("/StreetImg/Camera0/Image_0000/001_235959000.jpg"));
        result = service.check(QVector<hnRoutePhotoProject>() << project, 2026);
        expect(result.errors.join(QStringLiteral("\n")).contains(QStringLiteral("图片缺失")), QStringLiteral("missing photo blocked"));
        stop = true;
        result = service.check(QVector<hnRoutePhotoProject>() << project, 2026, this);
        expect(result.cancelled, QStringLiteral("cancel check"));
        stop = false;
    }
    void realProject(const QString& path, const QString& destination, int index)
    {
        hnRoutePhotoProject p;
        p.path = path;
        p.name = QFileInfo(path).fileName();
        QFile metadata(path + QStringLiteral("/ProjectInfo.txt"));
        expect(metadata.open(QIODevice::ReadOnly), QStringLiteral("real metadata read only"));
        const QString text = QString::fromUtf8(metadata.readAll());
        const QStringList lines = text.split(QLatin1Char('\n'));
        for (const QString& line : lines)
        {
            const QString key = line.section(QChar(0xff1a), 0, 0).trimmed();
            const QString value = line.section(QChar(0xff1a), 1).trimmed();
            if (key == QStringLiteral("工程起点道路编号"))
            {
                p.roadCode = value;
            }
            if (key == QStringLiteral("采集日期"))
            {
                p.date = QDate::fromString(value, QStringLiteral("yyyyMMdd"));
            }
            if (key == QStringLiteral("工程起点桩号") || key == QStringLiteral("工程终点道路标识桩号"))
            {
                QString pile = value;
                pile.remove(QLatin1Char('K'));
                const double mile = pile.section(QLatin1Char('+'), 0, 0).toDouble() * 1000
                    + pile.section(QLatin1Char('+'), 1).toDouble();
                if (key == QStringLiteral("工程起点桩号"))
                {
                    p.startMile = mile;
                }
                else
                {
                    p.endMile = mile;
                }
            }
        }
        hnRoutePhotoExportService service;
        const hnRoutePhotoCheck result = service.check(QVector<hnRoutePhotoProject>() << p, 2026);
        write(destination + QStringLiteral("/real-%1.txt").arg(index), result.errors.join(QStringLiteral("\n\n")).toUtf8());
        expect(!result.errors.isEmpty(), QStringLiteral("real project correctly blocked"));
        const QString errors = result.errors.join(QStringLiteral("\n"));
        if (p.roadCode == QStringLiteral("Y099511403"))
        {
            expect(errors.contains(QStringLiteral("GPS 桩号")) && !errors.contains(QStringLiteral("间隔异常"))
                && !errors.contains(QStringLiteral("StreetDis")), QStringLiteral("10m real project only GPS prerequisite"));
        }
        else
        {
            expect(errors.contains(QStringLiteral("StreetDis")), QStringLiteral("20m real project rejected"));
        }
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    RoutePhotoTests tests;
    if (app.arguments().size() < 2)
    {
        return 2;
    }
    QDir().mkpath(app.arguments()[1]);
    tests.run(app.arguments()[1]);
    for (int i = 2; i < app.arguments().size(); ++i)
    {
        tests.realProject(app.arguments()[i], app.arguments()[1], i - 1);
    }
    return tests.failures ? 1 : 0;
}
