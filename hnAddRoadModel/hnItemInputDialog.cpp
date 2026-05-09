#include "hnItemInputDialog.h"

hnItemInputDialog::hnItemInputDialog(QWidget *parent)
	: QDialog(parent),itemName_("")
{
	ui.setupUi(this);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &hnItemInputDialog::setDis);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &hnItemInputDialog::hide);
	connect(ui.comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(changeStrengSelect(int)));

}

hnItemInputDialog::~hnItemInputDialog()
{
}

void hnItemInputDialog::setDis()
{
	/*if (ui.radioButton->isChecked())
	{
		_userDis.drawtype = 0;
	}
	else
	{
		_userDis.drawtype = 1;
	}
	_userDis.rodaType = ui.comboBox->currentIndex();
	_userDis.disType = ui.comboBox_2->currentIndex();
	_userDis.diseaseName = ui.itemInputEdit->text();*/
	setDisStr();
	this->hide();
}

void hnItemInputDialog::setDisStr()
{
	if (ui.radioButton->isChecked())
	{
		itemName_ = ui.radioButton->text()+"_";
	}
	else
	{
		itemName_ = ui.radioButton_2->text() + "_";
	}
	itemName_ += ui.comboBox->currentText() + "_";
	if (!ui.radioButton_6->isChecked())
	{
		if (ui.radioButton_3->isChecked())
		{
			itemName_ += QStringLiteral("Çá_");
		}
		else if (ui.radioButton_4->isChecked())
		{
			itemName_ += QStringLiteral("ÖÐ_");
		}
		else
			itemName_ += QStringLiteral("ÖØ_");
	}
	if (ui.comboBox_2->currentIndex()!=0)
	{
		itemName_ += ui.comboBox_2->currentText() + "_";

	}
	itemName_ += ui.itemInputEdit->text();
	
}

void hnItemInputDialog::clearDst()
{
	this->itemName_ = "";
	ui.itemInputEdit->setText("");
}

void hnItemInputDialog::changeStrengSelect(int index)
{
	/*if (index!=0)
	{
		ui.radioButton_6->setChecked(true);
	}*/
}

QString hnItemInputDialog::getDis()
{
	return itemName_;
}

