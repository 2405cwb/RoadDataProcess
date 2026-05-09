#include "hnRoadItemInputDialog.h"

hnRoadItemInputDialog::hnRoadItemInputDialog(QWidget *parent)
	: QDialog(parent), itemName_("")
{
	ui.setupUi(this);

	ui.radioButton->setEnabled(false);
	ui.radioButton_2->setEnabled(false);
	ui.comboBox->setEnabled(false);
	ui.itemInputEdit->setEnabled(false);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &hnRoadItemInputDialog::setDis);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &hnRoadItemInputDialog::hide);

	//connect(ui.radioButton_4, &QRadioButton::toggled, this, &hnRoadItemInputDialog::handelChange);
	connect(ui.radioButton_4, SIGNAL(toggled(bool)), this, SLOT(handelChange(bool)));
}

hnRoadItemInputDialog::~hnRoadItemInputDialog()
{
	
}

void hnRoadItemInputDialog::cleartxt()
{
	this->itemName_ = "";
}

QString hnRoadItemInputDialog::getTxt()
{
	return itemName_;
}

void hnRoadItemInputDialog::setDis()
{
	if (ui.radioButton_4->isChecked())
	{
		itemName_ = QStringLiteral("模块道路公共参数");
	}
	else
	{
		if (ui.radioButton->isChecked())
		{
			itemName_ = ui.radioButton->text() + "_";
		}
		else
		{
			itemName_ = ui.radioButton_2->text() + "_";
		}
		itemName_ += ui.comboBox->currentText() + "_";
		//itemName_ += ui.comboBox_2->currentText() + "_";
		itemName_ += ui.itemInputEdit->text();
		// 人工模式_沥青_一级公路
	}
	this->hide();
	
}

void hnRoadItemInputDialog::handelChange(bool yes)
{
	if (yes)
	{
		ui.radioButton->setEnabled(false);
		ui.radioButton_2->setEnabled(false);
		ui.comboBox->setEnabled(false);
		ui.itemInputEdit->setEnabled(false);
	}
	else
	{
		ui.radioButton->setEnabled(true);
		ui.radioButton_2->setEnabled(true);
		ui.comboBox->setEnabled(true);
		ui.itemInputEdit->setEnabled(true);
	}
}
