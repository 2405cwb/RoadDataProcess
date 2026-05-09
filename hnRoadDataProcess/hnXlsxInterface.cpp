#include "hnXlsxInterface.h"

hnXlsxInterface::hnXlsxInterface(QObject *parent)
	: QObject(parent)
{
	//初始化内容的样式
	this->m_contentFormat.setHorizontalAlignment(Format::AlignHCenter);
	this->m_contentFormat.setVerticalAlignment(Format::AlignVCenter);
	this->m_contentFormat.setBorderStyle(Format::BorderThin);
	this->m_contentFormat.setFontName(QString::fromLocal8Bit("等线"));
	this->m_contentFormat.setFontBold(false);
	this->m_contentFormat.setFontSize(12);
	
}

hnXlsxInterface::~hnXlsxInterface()
{
}

Format hnXlsxInterface::getContentFormat()
{
	return m_contentFormat;
}

bool hnXlsxInterface::saveExcelFile(Document & const xlsx, const QString & excelFileAbsuluteName)
{
	//保存到文件
	QFile outputFile(excelFileAbsuluteName);
	if (outputFile.exists())
	{
		outputFile.remove();
	}

	if (!xlsx.saveAs(excelFileAbsuluteName))
	{
		return false;
	}

	return true;
}

