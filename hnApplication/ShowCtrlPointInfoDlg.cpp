#include "ShowCtrlPointInfoDlg.h"

ShowCtrlPointInfoDlg::ShowCtrlPointInfoDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

ShowCtrlPointInfoDlg::~ShowCtrlPointInfoDlg()
{
}

void ShowCtrlPointInfoDlg::setName(const QString & name)
{
	ui.nameLabel->setText(name);
}

void ShowCtrlPointInfoDlg::setGpsTime(double time)
{
	QString gpsTime = QString::number(time, 'f', 3);
	ui.gpsTimeLabel->setText(gpsTime);
}

void ShowCtrlPointInfoDlg::setPixName(const QString & pixName)
{
	ui.pixNameLabel->setText(pixName);
}

void ShowCtrlPointInfoDlg::setPixCoord(int x, int y)
{
	QString pixCoord = QString("%1,%2").arg(x).arg(y);
	ui.pixCoordLabel->setText(pixCoord);
}

void ShowCtrlPointInfoDlg::setPix3dCoord(double x, double y, double z)
{
	QString pix3dCoord = QString("%1,%2,%3").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(z, 0, 'f', 2);
	ui.coord3dLabel->setText(pix3dCoord);
}
