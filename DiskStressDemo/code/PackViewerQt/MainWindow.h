#pragma once

#include "QtMsvcCompat.h"
#include "../PackSdk/qt/PackReaderQt.h"

#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTextEdit>
#include <memory>

class QCheckBox;
class QProgressDialog;
class QSplitter;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);

private slots:
	void chooseFolder();
	void openFolder();
	void frameSelectionChanged();
	void showPrevious();
	void showNext();
	void jumpToGlobalIndex();
	void jumpToSourceIndex();
	void saveCurrent();
	void exportAll();
	void verifyDataset();
	void zoomIn();
	void zoomOut();
	void actualSize();
	void fitToWindowChanged();

private:
	void buildUi();
	void setActionsEnabled(bool enabled);
	void loadDataset(const QString& rootDir);
	void populateFrameList();
	void showFrame(quint64 globalIndex);
	void updateImageView();
	void updateFrameInfo(const PACK_FRAME_INFO& info, int imageWidth, int imageHeight);
	QString frameDisplayName(const PACK_FRAME_INFO& info) const;
	QString formatTimeValue(quint64 timeValue) const;
	QString lastSdkError() const;
	void setStatus(const QString& text);

	static int PACKSDK_CALL exportCallback(
		uint64_t globalIndex,
		uint64_t totalCount,
		const wchar_t* outputPath,
		int result,
		void* userData);
	static int PACKSDK_CALL exportCallbackUtf8(
		uint64_t globalIndex,
		uint64_t totalCount,
		const char* outputPath,
		int result,
		void* userData);
	static int handleExportCallback(
		uint64_t globalIndex,
		uint64_t totalCount,
		const QString& outputPath,
		int result,
		void* userData);

private:
	QLineEdit* m_folderEdit;
	QPushButton* m_chooseButton;
	QPushButton* m_openButton;
	QLineEdit* m_globalIndexEdit;
	QPushButton* m_jumpIndexButton;
	QLineEdit* m_sourceIndexEdit;
	QPushButton* m_jumpSourceButton;
	QListWidget* m_frameList;
	QLabel* m_imageLabel;
	QScrollArea* m_scrollArea;
	QTextEdit* m_infoText;
	QPushButton* m_prevButton;
	QPushButton* m_nextButton;
	QPushButton* m_saveButton;
	QPushButton* m_exportButton;
	QPushButton* m_verifyButton;
	QPushButton* m_zoomInButton;
	QPushButton* m_zoomOutButton;
	QPushButton* m_actualSizeButton;
	QCheckBox* m_fitCheck;

	std::unique_ptr<PackReaderQt> m_reader;
	QImage m_currentImage;
	QByteArray m_currentJpeg;
	quint64 m_currentIndex;
	double m_zoom;
	bool m_loadingList;
	QPointer<QProgressDialog> m_exportProgress;
};
