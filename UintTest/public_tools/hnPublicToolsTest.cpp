#include "hnDiseaseSampleExporter.h"
#include "hnReportMergeTool.h"
#include "hnReportMergeDlg.h"
#include "../hnDataTable/hnDBSqlite.h"
#include "xlsxdocument.h"
#include "xlsxformat.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QPixmap>
#include <sqlite3.h>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QTextStream out(stdout);
    QTemporaryDir temporary(QDir::currentPath() + "/fixtures-XXXXXX");
    if (!temporary.isValid()) return 1;
    QDir root(temporary.path());
    QString source = root.filePath("source.db");
    hnDBSqlite db(source.toLocal8Bit().constData());
    if (!db.connectDB(vector<string>())) return 2;
    QByteArray fixtureSql("CREATE TABLE LegacyDisease(ID INTEGER PRIMARY KEY,DiseaseType INTEGER,AddFile4 TEXT,Shape BLOB);"
        "INSERT INTO LegacyDisease VALUES(1,0,'0',X'00FF80'),(2,0,'',X'1122'),(3,0,NULL,X'33'),(4,0,'1',X'44'),"
        "(5,0,'2',X'55'),(6,0,'3',X'66'),(7,1,'0',X'77'),(8,2,'3',X'88'),(9,3,'2',X'99');"
        "CREATE TABLE Metadata(Key TEXT,Value BLOB);INSERT INTO Metadata VALUES('bytes',X'B5C0C2B700FF');"
        "CREATE INDEX SampleIndex ON LegacyDisease(AddFile4);");
    for (const QByteArray& sql : fixtureSql.split(';'))
    {
        if (!sql.trimmed().isEmpty() && !db.executeDB(sql.constData())) return 3;
    }
    QFile original(source);
    original.open(QIODevice::ReadOnly);
    QByteArray before = original.readAll(); original.close();
    hnDiseaseSampleExporter exporter;
    QString manual, negative, error;
    qint64 count = -1;
    if (!exporter.exportDatabase(&db, root.path(), false, manual, count, error) || count != 3)
    { out << "count=" << count << " " << error << endl; return 4; }
    if (!exporter.exportDatabase(&db, root.path(), true, negative, count, error) || count != 2) return 5;
    original.open(QIODevice::ReadOnly);
    if (original.readAll() != before) return 6;
    original.close();
    for (const QString& copy : {manual, negative})
    {
        sqlite3* sqlite = nullptr;
        sqlite3_open_v2(copy.toUtf8().constData(), &sqlite, SQLITE_OPEN_READONLY, nullptr);
        sqlite3_stmt* statement = nullptr;
        sqlite3_prepare_v2(sqlite, "SELECT hex(Value) FROM Metadata", -1, &statement, nullptr);
        bool ok = sqlite3_step(statement) == SQLITE_ROW && QByteArray(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0))) == "B5C0C2B700FF";
        sqlite3_finalize(statement);
        sqlite3_prepare_v2(sqlite, "SELECT count(*) FROM sqlite_master WHERE name='SampleIndex'", -1, &statement, nullptr);
        ok = ok && sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 1;
        sqlite3_finalize(statement);
        sqlite3_close(sqlite);
        if (!ok) return 7;
    }
    out << "PASS manual/deleted selection, metadata bytes, schema and source unchanged\n";
    if (!db.executeDB("CREATE TRIGGER prevent_delete BEFORE DELETE ON LegacyDisease BEGIN SELECT RAISE(ABORT,'test'); END;")) return 8;
    int dirs = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    QString failed;
    if (exporter.exportDatabase(&db, root.path(), false, failed, count, error) || !failed.isEmpty() || error.isEmpty() ||
        root.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size() != dirs) return 9;
    out << "PASS database failure publishes no partial output\n";

    QStringList files;
    for (int i = 1; i <= 2; i++)
    {
        QXlsx::Document book;
        book.renameSheet("Sheet1", "Data");
        QXlsx::Format format;
        format.setFontBold(true);
        format.setPatternBackgroundColor(QColor("#dceef7"));
        book.write(1, 1, "Heading", format);
        book.mergeCells("A1:C1", format);
        book.write(2, 1, "ID"); book.write(2, 2, "Value"); book.write(2, 3, "Total");
        book.write(3, 1, i); book.write(3, 2, 10 * i); book.write(3, 3, "=B3*2");
        book.addSheet("Other");
        QString name = root.filePath(QString("G00%1_PCI.xlsx").arg(i));
        if (!book.saveAs(name)) return 10;
        files.append(name);
    }
    QFile::copy(files.first(), root.filePath(QString::fromUtf8("PCI_合并结果.xlsx")));
    QFile::copy(files.first(), root.filePath("~$PCI.xlsx"));
    hnReportMergeTool scan;
    if (scan.findReports(root.path(), "PCI") != files) return 11;
    out << "PASS excludes previous outputs and temporary workbooks\n";
    QString output = root.filePath("merged.xlsx");
    {
        hnReportMergeTool merge;
        if (!merge.merge(files, "Data", 3, false, output, error, {}))
        { out << "FAIL merge " << error << "\n"; out.flush(); return 12; }
    }
    QXlsx::Document merged(output);
    if (merged.sheetNames() != (QStringList() << "Data") || merged.read(4, 1).toInt() != 2 || merged.read(3, 2).toInt() != 10 || merged.read(4, 2).toInt() != 20) return 13;
    out << "PASS real Excel merge, worksheet selection and row order\n";
    QString codesOutput = root.filePath("codes.xlsx");
    {
        hnReportMergeTool merge;
        if (!merge.merge(files, "Data", 3, true, codesOutput, error, {})) return 14;
    }
    QXlsx::Document codes(codesOutput);
    if (codes.read(3, 1).toString() != "G001" || codes.read(4, 1).toString() != "G002" || codes.read(3, 2).toInt() != 1 || codes.read(4, 2).toInt() != 2) return 15;
    out << "PASS road codes and shifted data\n";
    {
        hnReportMergeTool merge;
        QString missing = root.filePath("missing.xlsx");
        if (merge.merge(files, "Missing", 3, false, missing, error, {}) || QFileInfo::exists(missing) || error.isEmpty()) return 16;
    }
    {
        hnReportMergeTool merge;
        QString cancelled = root.filePath("cancelled.xlsx");
        if (merge.merge(files, "Data", 3, false, cancelled, error, [](int current, int) { return current == 0; }) || QFileInfo::exists(cancelled)) return 17;
    }
    out << "PASS missing sheet and cancellation publish no result\n";
    hnReportMergeDlg dialog;
    dialog.show();
    app.processEvents();
    if (!dialog.grab().save("merge-dialog.png")) return 18;
    out << "PASS dialog rendering\n";
    out.flush();
    return 0;
}
