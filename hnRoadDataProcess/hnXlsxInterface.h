#pragma once

#include <QObject>
#include "xlsxdocument.h"
#include <QFile>
#include "xlsxcellrange.h"

QXLSX_USE_NAMESPACE

class hnXlsxInterface : public QObject
{
	Q_OBJECT

public:
	hnXlsxInterface(QObject *parent = nullptr);
	~hnXlsxInterface();

public:
	//获取表格内容样式
	Format getContentFormat();

	//保存excel文件
	bool saveExcelFile(Document &const xlsx, const QString &excelFileAbsuluteName);



private:
	//内容的样式
	Format m_contentFormat;	

};
