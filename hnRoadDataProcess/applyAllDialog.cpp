#include "applyAllDialog.h"
#include "../hnQtCommon/HnProjectEnums.h"
#include  "../hnApplication/hnDataManager.h"
applyAllDialog::applyAllDialog(const QStringList &roadTypeNames,QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	
	ui.roadTypeComboBox->addItems(roadTypeNames);
	QString defaultType = QString::fromLocal8Bit("等级公路2018");
	if (roadTypeNames.contains(defaultType))
	{
		ui.roadTypeComboBox->setCurrentText(defaultType);
	}

	ui.okPushButton->setFocus();

	connect(ui.roadTypeComboBox, &QComboBox::currentTextChanged, this, [&](const QString& standard)
	{
		HnProjectEnums::StandardParmTypeEnum roadStandardEnum = HnProjectEnums::roadTypeQStringToEnum(standard);

		QVector<QString> vector = hnApp::hnDataManager::getDataManager()->getDrawTypes(roadStandardEnum);
		QStringList stringList;
		for (auto &str : vector)
		{
			stringList.append(str);
		}
		ui.frameTypeComboBox->clear();
		ui.frameTypeComboBox->addItems(stringList);
	}
	);

	connect(ui.okPushButton, &QPushButton::clicked, this, &applyAllDialog::slot_okPushButtonClicked);
	connect(ui.cancelPushButton, &QPushButton::clicked, this, &applyAllDialog::slot_cancelPushButtonClicked);
}

applyAllDialog::~applyAllDialog()
{
}

QString applyAllDialog::getRoadType()
{
	return ui.roadTypeComboBox->currentText();
}

QString applyAllDialog::getFrameType()
{
	return ui.frameTypeComboBox->currentText();
}

QString applyAllDialog::getRoadWidth()
{
	return ui.roadWidthLineEdit->text();
}

void applyAllDialog::slot_okPushButtonClicked()
{
	this->accept();
}

void applyAllDialog::slot_cancelPushButtonClicked()
{
	this->reject();
}
