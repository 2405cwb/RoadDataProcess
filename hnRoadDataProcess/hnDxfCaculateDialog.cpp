#include "hnDxfCaculateDialog.h"
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>
#include "dxfExcelInfo.h"

hnDxfCaculateDialog::hnDxfCaculateDialog(QWidget *parent)
	: QDialog(parent)
{
	this->setWindowTitle(QString::fromLocal8Bit("DXF导出"));
	this->initUI();
}

hnDxfCaculateDialog::~hnDxfCaculateDialog()
{
}

void hnDxfCaculateDialog::initUI()
{
	m_upGroupBox = new QGroupBox(QString::fromLocal8Bit("上行（右幅）车道"));
	m_upGridLayout = new QGridLayout;
	this->initLaneExcelChooseWidgets(m_upGridLayout);
	m_upGroupBox->setLayout(m_upGridLayout);

	m_downGroupBox = new QGroupBox(QString::fromLocal8Bit("下行（左幅）车道"));
	m_downGridlayout = new QGridLayout;
	this->initLaneExcelChooseWidgets(m_downGridlayout);
	m_downGroupBox->setLayout(m_downGridlayout);
	
	m_allRadioButton = new QRadioButton(QString::fromLocal8Bit("全幅"));
	m_allRadioButton->setChecked(true);
	m_rightRadioButton = new QRadioButton(QString::fromLocal8Bit("上行右半幅"));
	m_leftRadioButton = new QRadioButton(QString::fromLocal8Bit("下行左半幅"));
	m_nationProvincialRoadCheckBox = new QCheckBox(QString::fromLocal8Bit("国省道"));
	connect(m_nationProvincialRoadCheckBox, &QCheckBox::stateChanged, this, &hnDxfCaculateDialog::slot_nationProvincialRoadCheckBoxStateChanged);
	m_beginMileLineEdit = new QLineEdit;
	m_beginMileLineEdit->setText("0");
	m_endMileLineEdit = new QLineEdit;
	m_endMileLineEdit->setText("0");
	m_okPushButton = new QPushButton(QString::fromLocal8Bit("确定"));
	m_okPushButton->setMinimumWidth(100);
	connect(m_okPushButton, &QPushButton::clicked, this, &hnDxfCaculateDialog::slot_okPushButtonClicked);
	m_cancelPushButton = new QPushButton(QString::fromLocal8Bit("取消"));
	m_cancelPushButton->setMinimumWidth(100);
	connect(m_cancelPushButton, &QPushButton::clicked, this, &hnDxfCaculateDialog::slot_cancelPushButtonClicked);
	m_roadWidthLineEdit = new QLineEdit;
	m_roadWidthLineEdit->setText("3.75");

	m_optionHBoxLayout = new QHBoxLayout;
	m_optionHBoxLayout->addWidget(m_allRadioButton);
	m_optionHBoxLayout->addWidget(m_rightRadioButton);
	m_optionHBoxLayout->addWidget(m_leftRadioButton);
	m_optionHBoxLayout->addWidget(m_nationProvincialRoadCheckBox);
	m_optionHBoxLayout->addWidget(new QLabel(QString::fromLocal8Bit("开始里程")));
	m_optionHBoxLayout->addWidget(m_beginMileLineEdit);
	m_optionHBoxLayout->addWidget(new QLabel(QString::fromLocal8Bit("结束里程")));
	m_optionHBoxLayout->addWidget(m_endMileLineEdit);
	m_optionHBoxLayout->addWidget(new QLabel(QString::fromLocal8Bit("路面宽度")));
	m_optionHBoxLayout->addWidget(m_roadWidthLineEdit);
	m_optionHBoxLayout->addWidget(m_okPushButton);
	m_optionHBoxLayout->addWidget(m_cancelPushButton);
	
	m_mainVBoxLineEdit = new QVBoxLayout;
	m_mainVBoxLineEdit->addWidget(m_upGroupBox);
	m_mainVBoxLineEdit->addWidget(m_downGroupBox);
	m_mainVBoxLineEdit->addLayout(m_optionHBoxLayout);

	this->setLayout(m_mainVBoxLineEdit);

}

void hnDxfCaculateDialog::initLaneExcelChooseWidgets(QGridLayout * layout)
{
	int row = 0;
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("一车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("二车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("三车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("四车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("五车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("六车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("七车道"));
	this->addLaneRowWidgets(layout, row++, QString::fromLocal8Bit("八车道"));
}

void hnDxfCaculateDialog::addLaneRowWidgets(QGridLayout * layout, int row, const QString & labelText)
{
	QLabel *laneNameLabel = new QLabel(labelText);
	QPushButton *chooseExcelPushButton = new QPushButton(QString::fromLocal8Bit("选择报表文件"));
	connect(chooseExcelPushButton, &QPushButton::clicked, this, &hnDxfCaculateDialog::slot_chooseExcelPushButtonCliecked);
	QLineEdit *excelPathLineEdit = new QLineEdit;
	excelPathLineEdit->setEnabled(false);
	if (row != 0)
	{
		chooseExcelPushButton->setEnabled(false);
	}

	layout->addWidget(laneNameLabel, row, 0);
	layout->addWidget(chooseExcelPushButton, row, 1);
	layout->addWidget(excelPathLineEdit, row, 2);
}

void hnDxfCaculateDialog::updatePushButtonState(QGridLayout * layout, int row)
{
	for (int i = row; i < layout->rowCount(); i++)
	{
		if (i < 0)
		{
			continue;
		}

		QPushButton *pushButton = qobject_cast<QPushButton*> (layout->itemAtPosition(i, 1)->widget());
		if (!pushButton)
		{		
			continue;
		}
		if (i == row || i == row + 1)
		{
			pushButton->setEnabled(true);
		}
		else
		{
			pushButton->setEnabled(false);
		}
	}
}

void hnDxfCaculateDialog::updateLineEditState(QGridLayout * layout, int row)
{
	for (int i = row + 1; i < layout->rowCount(); i++)
	{
		if (i < 0)
		{
			continue;
		}

		QLineEdit *lineEdit = qobject_cast<QLineEdit*> (layout->itemAtPosition(i, 2)->widget());
		if (!lineEdit)
		{
			continue;
		}
		lineEdit->clear();
	}
}

void hnDxfCaculateDialog::slot_chooseExcelPushButtonCliecked()
{
	//定位到发送者的pushButton
	QString excelFileName = QFileDialog::getOpenFileName(this,QString::fromLocal8Bit("请选择报表文件"),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),"xls(*.xls *.xlsx)");

	QPushButton *senderPushButton = qobject_cast<QPushButton*>(sender());

	//找到这个按钮的索引,确定按钮在哪个布局中
	int pushButtonIndex = -1;
	QGridLayout *layout = m_upGridLayout;
	pushButtonIndex = m_upGridLayout->indexOf(senderPushButton);
	if (pushButtonIndex == -1)
	{
		layout = m_downGridlayout;
	}

	pushButtonIndex = layout->indexOf(senderPushButton);
	
	//找到控件的行列，确定lineedit的位置
	int row, column ,rowSpan, columnSpan;
	QLineEdit *excelFilePathLineEdit;

	layout->getItemPosition(pushButtonIndex, &row, &column, &rowSpan, &columnSpan);
	excelFilePathLineEdit = qobject_cast<QLineEdit*> (layout->itemAtPosition(row, column + 1)->widget());

	//给lineedit赋值
	if (excelFilePathLineEdit)
	{
		excelFilePathLineEdit->setText(excelFileName);
	}

	if (!excelFileName.isEmpty())
	{
		//更新按钮状态
		this->updatePushButtonState(layout, row);
		//更新lineEdit状态
		this->updateLineEditState(layout, row);
		//获取开始里程和结束里程
		int beginMile = 0;
		int endMile = 0;
		dxfExcelInfo::getExcelMiles(excelFileName, beginMile, endMile);
		int maxMile = qMax(beginMile, endMile);
		int minMile = qMin(beginMile, endMile);
		m_beginMileLineEdit->setText(QString::number(minMile));
		m_endMileLineEdit->setText(QString::number(maxMile));
	}	
}

void hnDxfCaculateDialog::slot_cancelPushButtonClicked()
{

	this->reject();
}

void hnDxfCaculateDialog::slot_okPushButtonClicked()
{
	if (m_beginMileLineEdit->text() == "0" && m_endMileLineEdit->text() == "0")
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("未填写里程，请重新操作"));
		return;
	}

	QString saveDirName = QFileDialog::getExistingDirectory(nullptr,QString::fromLocal8Bit("请选择输出文件夹"),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
	if (saveDirName.isEmpty())
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("您没有选择路径，请重新操作"));
		return;
	}

	if (true == m_nationProvincialRoadCheckBox->checkState())
	{
		this->nationProvincialRoadMessageBoxInfo();
	}

	int rc = -1;
	//全幅
	if (m_allRadioButton->isChecked())
	{
		rc = this->preProcessAllFile(saveDirName);
	}
	//下行左幅
	else if (m_leftRadioButton->isChecked())
	{
		rc = this->preProcessDownFile(saveDirName);
	}
	//上行右幅
	else if (m_rightRadioButton->isChecked())
	{
		rc = this->preprocessUpFile(saveDirName);
	}
	
	// TODO 这里简单判断一下，后面要把所有的错误信息展示给用户
	if (0 == rc)
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("导出dxf成功"));
		return;
	}
	else
	{
		QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("导出dxf失败"));
		return;
	}
	
}

void hnDxfCaculateDialog::slot_nationProvincialRoadCheckBoxStateChanged(bool state)
{
	if (true == state)
	{
		this->nationProvincialRoadMessageBoxInfo();
	}
}

void hnDxfCaculateDialog::nationProvincialRoadMessageBoxInfo()
{
	QMessageBox::information(this, QString::fromLocal8Bit("提示"),
		QString::fromLocal8Bit("国省道输出dxf，只支持等级公路2018病害。"
			"国省道输出耗时较长，请耐心等待"));
}

int hnDxfCaculateDialog::preprocessUpFile(const QString &saveDirName)
{

	auto fileNames = this->getExcelFileNames(m_upGridLayout);

	//把界面中包含的信息交由专门的类处理
	hnDxfInfoPreprocess process;

	int rc = process.preprocess(m_nationProvincialRoadCheckBox->isChecked(), fileNames, m_beginMileLineEdit->text().toDouble(),
		m_endMileLineEdit->text().toDouble(), saveDirName, m_roadWidthLineEdit->text().toDouble(), 1);

	return rc;
}

int hnDxfCaculateDialog::preProcessDownFile(const QString & saveDirName)
{
	auto fileNames = this->getExcelFileNames(m_downGridlayout);

	//把界面中包含的信息交由专门的类处理
	hnDxfInfoPreprocess process;

	int rc = process.preprocess(m_nationProvincialRoadCheckBox->isChecked(), fileNames, m_beginMileLineEdit->text().toDouble(),
		m_endMileLineEdit->text().toDouble(), saveDirName, m_roadWidthLineEdit->text().toDouble(), -1);

	return rc;
}

int hnDxfCaculateDialog::preProcessAllFile(const QString & saveDirName)
{
	auto upfileNames = this->getExcelFileNames(m_upGridLayout);

	auto downFileNames = this->getExcelFileNames(m_downGridlayout);
	std::reverse(downFileNames.begin(), downFileNames.end());

	if (upfileNames.size() != downFileNames.size())
	{
		return -9;
	}

	const QStringList fileNames = downFileNames + upfileNames;

	//把界面中包含的信息交由专门的类处理
	hnDxfInfoPreprocess process;

	//dxf输出那里没有全幅一说，暂时按上行（大于0)计算
	const int lineType = 1;

	int rc = process.preprocess(m_nationProvincialRoadCheckBox->isChecked(), fileNames, m_beginMileLineEdit->text().toDouble(),
		m_endMileLineEdit->text().toDouble(), saveDirName, m_roadWidthLineEdit->text().toDouble(), lineType);

	return rc;
}

QStringList hnDxfCaculateDialog::getExcelFileNames()
{
	QStringList fileNames;
	QStringList upFileNames = this->getExcelFileNames(m_upGridLayout);
	QStringList downFileNames = this->getExcelFileNames(m_downGridlayout);

	if (m_allRadioButton->isChecked())
	{
		if (upFileNames.isEmpty() || downFileNames.isEmpty())
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
				QString::fromLocal8Bit("您选择的是全幅，但是全幅的表格信息没有填写完整，请重新选择"));
			return fileNames;
		}
		else
		{
			fileNames = upFileNames + downFileNames;
			return fileNames;
		}
	}
	else if (m_rightRadioButton->isChecked())
	{
		if (upFileNames.isEmpty())
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
				QString::fromLocal8Bit("您选择的是上行右半幅，但是上行右半幅的表格信息没有填写完整，请重新选择"));
			return fileNames;
		}
		else
		{
			fileNames = upFileNames;
			return fileNames;
		}
	}
	else if (m_leftRadioButton->isChecked())
	{
		if (downFileNames.isEmpty())
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
				QString::fromLocal8Bit("您选择的是下行左半幅，但是下行左半幅的表格信息没有填写完整，请重新选择"));
			return fileNames;
		}
		else
		{
			fileNames = downFileNames;
			return fileNames;
		}
	}
	

	return fileNames;
}

QStringList hnDxfCaculateDialog::getExcelFileNames(QGridLayout * layout)
{
	QStringList fileNames;
	if (!layout)
	{
		return fileNames;
	}
	
	for (int i = 0; i < layout->count(); i++)
	{
		QLineEdit *lineEdit = qobject_cast<QLineEdit*>(layout->itemAt(i)->widget());
		if (lineEdit)
		{
			QString text = lineEdit->text();
			if (!text.isEmpty())
			{
				fileNames.push_back(text);
			}
		}
	}

	return fileNames;
}


