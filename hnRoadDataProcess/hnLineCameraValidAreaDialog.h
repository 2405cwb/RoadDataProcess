#pragma once

#include <QDialog>
#include "../hnProject/hnLineCameraConfig.h"

class hnLineCameraValidAreaDialog : public QDialog
{
public:
	explicit hnLineCameraValidAreaDialog(const hnPro::hnLineCameraInfo& info, QWidget* parent = nullptr);
	hnPro::hnLineCameraInfo selectedInfo() const;

private:
	class PreviewView;
	void updateSummary();
	bool applyPixelInputs();
	bool applyRoadWidthInput();
	bool applyPendingInput();

	hnPro::hnLineCameraInfo m_info;
	PreviewView* m_previewView = nullptr;
	class QLabel* m_summaryLabel = nullptr;
	class QLineEdit* m_leftPixelEdit = nullptr;
	class QLineEdit* m_rightPixelEdit = nullptr;
	class QLineEdit* m_roadWidthEdit = nullptr;
	int m_pendingInputMode = 0;
};
