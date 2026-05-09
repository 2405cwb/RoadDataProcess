#pragma once

#include "hnio_global.h"
#include "../ActiveQt/QAxObject"
#include <QString>
#include <windows.h>
class HNIO_EXPORT hnExcelIO
{
public:
	hnExcelIO(QString strExcelPath, QString strSavePath);
	~hnExcelIO();

	bool OpenExcel();
	void AddNewExcel();
	void SaveAndClose();
	int GetRowsCount();
	int GetColumnsCount();
	int GetTablesCount();

	QString GetCell(int row, int column);
	QString GetCell(QString numer);

	void SetCell(int row, int column, QString value);
	void SetCell(QString number, QString value);
	void SetCellColor(int row, int column, QColor color);
	void SetCellColor(QString number, QColor color);

	void deleteRows(UINT RowNum);
	QString getSheetName(const int num);
	void removeSheet(const int num);

	void appendSheet(const QString& sheetName);

	int GetUserStartRowIndex();
	int GetUserStartColumnIndex();
	int getUserAllRowsCount();
	int getUserAllColumnsCount();

	QVariant readAll();

	QVariant readAll(QString para1, QString para2);

	bool DelDir(const QString path);
private:
	hnExcelIO();
	QString _strExcelPath;
	QString _strSavePath;
	QAxObject* m_pExcel;
	QAxObject* m_pWorkBooks;
	QAxObject* m_pWorkBook;
	QAxObject* m_pWorkSheets;
	QAxObject* m_pWorkSheet;
	QAxObject* usedRange;
	int userStartRowIndex;
	int userStartColumnIndex;
	int userAllRowsCount;
	int userAllColumnsCount;
};

