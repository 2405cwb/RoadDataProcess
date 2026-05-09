#pragma once

#include <QDialog>
#include "ui_hnMergeLittleFrameThresholdDlg.h"

class hnMergeLittleFrameThresholdDlg : public QDialog
{
	Q_OBJECT

public:
	hnMergeLittleFrameThresholdDlg(QWidget *parent = Q_NULLPTR);
	~hnMergeLittleFrameThresholdDlg();

	int hDisThreshold();

	int vDisThreshold();

	int importMergeFlag();

	int importMapFlag();

private:

	// 水平方向距离阈值
	int m_hDisThreshold;

	// 垂直方向距离阈值
	int m_vDisThreshold;

	// 导入病害时是否合并病害
	bool m_isMerge;

	// 导入病害时是否映射病害
	bool m_isMap;

public:
	Ui::hnMergeLittleFrameThresholdDlg ui;
};
