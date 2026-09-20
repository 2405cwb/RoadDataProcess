#include "MainWindow.h" 
#include <QApplication>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImageReader>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPixmap>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QVariant>
#include <QVBoxLayout>

namespace
{
struct ExportContext
{
	MainWindow* window;
	QElapsedTimer timer;
	quint64 exportedFrames;
	quint64 exportedBytes;
	quint64 failedFrames;
	qint64 lastUiUpdateMs;
	QString lastOutputPath;
	int lastResult;
	bool cancelled;

	ExportContext(MainWindow* owner)
		: window(owner)
		, exportedFrames(0)
		, exportedBytes(0)
		, failedFrames(0)
		, lastUiUpdateMs(0)
		, lastResult(PACK_OK)
		, cancelled(false)
	{
	}
};

QString formatBytes(quint64 bytes)
{
	const double kb = 1024.0;
	const double mb = kb * 1024.0;
	const double gb = mb * 1024.0;
	if (bytes >= static_cast<quint64>(gb))
	{
		return QStringLiteral("%1 GB").arg(static_cast<double>(bytes) / gb, 0, 'f', 2);
	}
	if (bytes >= static_cast<quint64>(mb))
	{
		return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / mb, 0, 'f', 2);
	}
	if (bytes >= static_cast<quint64>(kb))
	{
		return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / kb, 0, 'f', 2);
	}
	return QStringLiteral("%1 B").arg(bytes);
}

QString formatExportSummary(const ExportContext& context, quint64 totalCount, const QString& outputDir)
{
	const double elapsedSec = qMax(0.001, context.timer.elapsed() / 1000.0);
	const double fps = context.exportedFrames / elapsedSec;
	const double mbps = (static_cast<double>(context.exportedBytes) / (1024.0 * 1024.0)) / elapsedSec;
	const double avgMs = context.exportedFrames > 0 ? context.timer.elapsed() / static_cast<double>(context.exportedFrames) : 0.0;

	QString text;
	text += QStringLiteral("批量导出统计\n");
	text += QStringLiteral("输出目录：%1\n").arg(outputDir);
	text += QStringLiteral("计划导出：%1 张\n").arg(totalCount);
	text += QStringLiteral("成功导出：%1 张\n").arg(context.exportedFrames);
	text += QStringLiteral("失败数量：%1 张\n").arg(context.failedFrames);
	text += QStringLiteral("导出字节：%1 (%2 bytes)\n").arg(formatBytes(context.exportedBytes)).arg(context.exportedBytes);
	text += QStringLiteral("总耗时：%1 秒\n").arg(elapsedSec, 0, 'f', 3);
	text += QStringLiteral("平均速度：%1 张/秒\n").arg(fps, 0, 'f', 2);
	text += QStringLiteral("吞吐量：%1 MB/秒\n").arg(mbps, 0, 'f', 2);
	text += QStringLiteral("平均每张：%1 ms\n").arg(avgMs, 0, 'f', 3);
	text += QStringLiteral("最后文件：%1\n").arg(context.lastOutputPath);
	text += QStringLiteral("最后结果码：%1").arg(context.lastResult);
	return text;
}
}

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, m_folderEdit(nullptr)
	, m_chooseButton(nullptr)
	, m_openButton(nullptr)
	, m_globalIndexEdit(nullptr)
	, m_jumpIndexButton(nullptr)
	, m_sourceIndexEdit(nullptr)
	, m_jumpSourceButton(nullptr)
	, m_frameList(nullptr)
	, m_imageLabel(nullptr)
	, m_scrollArea(nullptr)
	, m_infoText(nullptr)
	, m_prevButton(nullptr)
	, m_nextButton(nullptr)
	, m_saveButton(nullptr)
	, m_exportButton(nullptr)
	, m_verifyButton(nullptr)
	, m_zoomInButton(nullptr)
	, m_zoomOutButton(nullptr)
	, m_actualSizeButton(nullptr)
	, m_fitCheck(nullptr)
	, m_currentIndex(0)
	, m_zoom(1.0)
	, m_loadingList(false)
{
	buildUi();
	setActionsEnabled(false);
	setWindowTitle(QStringLiteral("Pack 图片浏览器 - PackSdk 示例"));
}

void MainWindow::buildUi()
{
	QWidget* root = new QWidget(this);
	QVBoxLayout* rootLayout = new QVBoxLayout(root);

	QHBoxLayout* topLayout = new QHBoxLayout();
	m_folderEdit = new QLineEdit(root);
	m_folderEdit->setPlaceholderText(QStringLiteral("选择包含 Pack_000000.dat / Pack_000000.idx 的输出目录"));
	m_chooseButton = new QPushButton(QStringLiteral("选择图片包文件夹"), root);
	m_openButton = new QPushButton(QStringLiteral("打开"), root);
	topLayout->addWidget(m_folderEdit, 1);
	topLayout->addWidget(m_chooseButton);
	topLayout->addWidget(m_openButton);
	rootLayout->addLayout(topLayout);

	QHBoxLayout* navLayout = new QHBoxLayout();
	m_prevButton = new QPushButton(QStringLiteral("上一张"), root);
	m_nextButton = new QPushButton(QStringLiteral("下一张"), root);
	m_globalIndexEdit = new QLineEdit(root);
	m_globalIndexEdit->setPlaceholderText(QStringLiteral("全局序号，从 0 开始"));
	m_jumpIndexButton = new QPushButton(QStringLiteral("跳转序号"), root);
	m_sourceIndexEdit = new QLineEdit(root);
	m_sourceIndexEdit->setPlaceholderText(QStringLiteral("sourceIndex"));
	m_jumpSourceButton = new QPushButton(QStringLiteral("跳转帧号"), root);
	m_saveButton = new QPushButton(QStringLiteral("保存当前图"), root);
	m_exportButton = new QPushButton(QStringLiteral("批量导出"), root);
	m_verifyButton = new QPushButton(QStringLiteral("校验数据"), root);
	m_zoomInButton = new QPushButton(QStringLiteral("放大"), root);
	m_zoomOutButton = new QPushButton(QStringLiteral("缩小"), root);
	m_actualSizeButton = new QPushButton(QStringLiteral("1:1"), root);
	m_fitCheck = new QCheckBox(QStringLiteral("适应窗口"), root);
	m_fitCheck->setChecked(true);

	navLayout->addWidget(m_prevButton);
	navLayout->addWidget(m_nextButton);
	navLayout->addWidget(m_globalIndexEdit);
	navLayout->addWidget(m_jumpIndexButton);
	navLayout->addWidget(m_sourceIndexEdit);
	navLayout->addWidget(m_jumpSourceButton);
	navLayout->addWidget(m_saveButton);
	navLayout->addWidget(m_exportButton);
	navLayout->addWidget(m_verifyButton);
	navLayout->addStretch(1);
	navLayout->addWidget(m_zoomOutButton);
	navLayout->addWidget(m_zoomInButton);
	navLayout->addWidget(m_actualSizeButton);
	navLayout->addWidget(m_fitCheck);
	rootLayout->addLayout(navLayout);

	QSplitter* splitter = new QSplitter(root);
	m_frameList = new QListWidget(splitter);
	m_frameList->setMinimumWidth(360);

	QWidget* rightPane = new QWidget(splitter);
	QVBoxLayout* rightLayout = new QVBoxLayout(rightPane);
	m_scrollArea = new QScrollArea(rightPane);
	m_scrollArea->setWidgetResizable(true);
	m_imageLabel = new QLabel(m_scrollArea);
	m_imageLabel->setAlignment(Qt::AlignCenter);
	m_imageLabel->setBackgroundRole(QPalette::Base);
	m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_scrollArea->setWidget(m_imageLabel);
	rightLayout->addWidget(m_scrollArea, 1);

	m_infoText = new QTextEdit(rightPane);
	m_infoText->setReadOnly(true);
	m_infoText->setFixedHeight(150);
	rightLayout->addWidget(m_infoText);

	splitter->addWidget(m_frameList);
	splitter->addWidget(rightPane);
	splitter->setStretchFactor(0, 0);
	splitter->setStretchFactor(1, 1);
	rootLayout->addWidget(splitter, 1);

	setCentralWidget(root);
	statusBar()->showMessage(QStringLiteral("请选择图片包文件夹"));

	connect(m_chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
	connect(m_openButton, &QPushButton::clicked, this, &MainWindow::openFolder);
	connect(m_frameList, &QListWidget::currentRowChanged, this, &MainWindow::frameSelectionChanged);
	connect(m_prevButton, &QPushButton::clicked, this, &MainWindow::showPrevious);
	connect(m_nextButton, &QPushButton::clicked, this, &MainWindow::showNext);
	connect(m_jumpIndexButton, &QPushButton::clicked, this, &MainWindow::jumpToGlobalIndex);
	connect(m_jumpSourceButton, &QPushButton::clicked, this, &MainWindow::jumpToSourceIndex);
	connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveCurrent);
	connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportAll);
	connect(m_verifyButton, &QPushButton::clicked, this, &MainWindow::verifyDataset);
	connect(m_zoomInButton, &QPushButton::clicked, this, &MainWindow::zoomIn);
	connect(m_zoomOutButton, &QPushButton::clicked, this, &MainWindow::zoomOut);
	connect(m_actualSizeButton, &QPushButton::clicked, this, &MainWindow::actualSize);
	connect(m_fitCheck, &QCheckBox::stateChanged, this, &MainWindow::fitToWindowChanged);
}

void MainWindow::setActionsEnabled(bool enabled)
{
	m_frameList->setEnabled(enabled);
	m_prevButton->setEnabled(enabled);
	m_nextButton->setEnabled(enabled);
	m_globalIndexEdit->setEnabled(enabled);
	m_jumpIndexButton->setEnabled(enabled);
	m_sourceIndexEdit->setEnabled(enabled);
	m_jumpSourceButton->setEnabled(enabled);
	m_saveButton->setEnabled(enabled);
	m_exportButton->setEnabled(enabled);
	m_verifyButton->setEnabled(enabled);
	m_zoomInButton->setEnabled(enabled);
	m_zoomOutButton->setEnabled(enabled);
	m_actualSizeButton->setEnabled(enabled);
	m_fitCheck->setEnabled(enabled);
}

void MainWindow::chooseFolder()
{
	QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择图片包文件夹"), m_folderEdit->text());
	if (!dir.isEmpty())
	{
		m_folderEdit->setText(dir);
		loadDataset(dir);
	}
}

void MainWindow::openFolder()
{
	loadDataset(m_folderEdit->text().trimmed());
}

void MainWindow::loadDataset(const QString& rootDir)
{
	if (rootDir.isEmpty())
	{
		QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请先选择图片包文件夹。"));
		return;
	}

	try
	{
		m_reader.reset(new PackReaderQt(rootDir));
		populateFrameList();
		setActionsEnabled(true);
		PACK_DATASET_INFO info = m_reader->datasetInfo();
		setStatus(QStringLiteral("打开成功：%1 个 pack，%2 张图片，warnings=%3")
			.arg(info.packCount)
			.arg(static_cast<qulonglong>(info.frameCount))
			.arg(static_cast<qulonglong>(info.warningCount)));
		if (info.frameCount > 0)
		{
			m_frameList->setCurrentRow(0);
		}
	}
	catch (const std::exception& ex)
	{
		m_reader.reset();
		m_frameList->clear();
		m_imageLabel->clear();
		m_infoText->clear();
		setActionsEnabled(false);
		QMessageBox::critical(this, QStringLiteral("打开失败"), QString::fromLocal8Bit(ex.what()));
	}
}

void MainWindow::populateFrameList()
{
	m_loadingList = true;
	m_frameList->clear();
	m_frameList->setUpdatesEnabled(false);
	quint64 count = m_reader->count();
	for (quint64 i = 0; i < count; ++i)
	{
		PACK_FRAME_INFO info = m_reader->frameInfo(i);
		QListWidgetItem* item = new QListWidgetItem(frameDisplayName(info), m_frameList);
		item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(i));
	}
	m_frameList->setUpdatesEnabled(true);
	m_loadingList = false;
}

void MainWindow::frameSelectionChanged()
{
	if (m_loadingList || m_reader.get() == nullptr)
	{
		return;
	}
	int row = m_frameList->currentRow();
	if (row < 0)
	{
		return;
	}
	QListWidgetItem* item = m_frameList->item(row);
	showFrame(item->data(Qt::UserRole).toULongLong());
}

void MainWindow::showFrame(quint64 globalIndex)
{
	if (m_reader.get() == nullptr)
	{
		return;
	}

	try
	{
		m_currentIndex = globalIndex;
		m_currentJpeg = m_reader->readJpeg(globalIndex);
		m_currentImage = QImage::fromData(m_currentJpeg, "JPG");
		if (m_currentImage.isNull())
		{
			throw std::runtime_error("JPEG 解码失败，SDK 已返回字节，但 Qt 无法识别为图片。");
		}
		PACK_FRAME_INFO info = m_reader->frameInfo(globalIndex);
		updateImageView();
		updateFrameInfo(info, m_currentImage.width(), m_currentImage.height());
		m_globalIndexEdit->setText(QString::number(globalIndex));
		m_sourceIndexEdit->setText(QString::number(static_cast<qulonglong>(info.sourceIndex)));
		setStatus(QStringLiteral("当前：%1 / %2").arg(globalIndex + 1).arg(m_reader->count()));
	}
	catch (const std::exception& ex)
	{
		QMessageBox::critical(this, QStringLiteral("读取失败"), QString::fromLocal8Bit(ex.what()));
	}
}

void MainWindow::updateImageView()
{
	if (m_currentImage.isNull())
	{
		m_imageLabel->clear();
		return;
	}

	QPixmap pixmap = QPixmap::fromImage(m_currentImage);
	if (m_fitCheck->isChecked())
	{
		QSize target = m_scrollArea->viewport()->size();
		m_imageLabel->setPixmap(pixmap.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		m_imageLabel->resize(target);
	}
	else
	{
		QSize scaledSize = pixmap.size() * m_zoom;
		m_imageLabel->setPixmap(pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		m_imageLabel->adjustSize();
	}
}

void MainWindow::updateFrameInfo(const PACK_FRAME_INFO& info, int imageWidth, int imageHeight)
{
	QString text;
	text += QStringLiteral("全局序号：%1\n").arg(static_cast<qulonglong>(info.globalIndex));
	text += QStringLiteral("采集帧号 sourceIndex：%1\n").arg(static_cast<qulonglong>(info.sourceIndex));
	text += QStringLiteral("采集时间 timeValue：%1 (%2)\n")
		.arg(static_cast<qulonglong>(info.timeValue))
		.arg(formatTimeValue(info.timeValue));
	text += QStringLiteral("Pack：%1，包内序号：%2\n").arg(info.packNo).arg(info.packFrameIndex);
	text += QStringLiteral("datOffset：%1，jpgSize：%2 bytes\n")
		.arg(static_cast<qulonglong>(info.datOffset))
		.arg(info.jpgSize);
	text += QStringLiteral("图片尺寸：%1 x %2，当前 JPEG 字节：%3\n")
		.arg(imageWidth)
		.arg(imageHeight)
		.arg(m_currentJpeg.size());
	text += QStringLiteral("状态标记：%1").arg(info.statusFlags);
	m_infoText->setPlainText(text);
}

QString MainWindow::frameDisplayName(const PACK_FRAME_INFO& info) const
{
	return QStringLiteral("%1 | src=%2 | %3 | %4 KB")
		.arg(static_cast<qulonglong>(info.globalIndex), 6, 10, QLatin1Char('0'))
		.arg(static_cast<qulonglong>(info.sourceIndex))
		.arg(formatTimeValue(info.timeValue))
		.arg(info.jpgSize / 1024);
}

QString MainWindow::formatTimeValue(quint64 timeValue) const
{
	QString s = QString::number(timeValue);
	if (s.size() != 17)
	{
		return s;
	}
	return QStringLiteral("%1-%2-%3 %4:%5:%6.%7")
		.arg(s.mid(0, 4))
		.arg(s.mid(4, 2))
		.arg(s.mid(6, 2))
		.arg(s.mid(8, 2))
		.arg(s.mid(10, 2))
		.arg(s.mid(12, 2))
		.arg(s.mid(14, 3));
}

void MainWindow::showPrevious()
{
	if (m_reader.get() == nullptr || m_currentIndex == 0)
	{
		return;
	}
	m_frameList->setCurrentRow(static_cast<int>(m_currentIndex - 1));
}

void MainWindow::showNext()
{
	if (m_reader.get() == nullptr || m_currentIndex + 1 >= m_reader->count())
	{
		return;
	}
	m_frameList->setCurrentRow(static_cast<int>(m_currentIndex + 1));
}

void MainWindow::jumpToGlobalIndex()
{
	if (m_reader.get() == nullptr)
	{
		return;
	}
	bool ok = false;
	quint64 index = m_globalIndexEdit->text().trimmed().toULongLong(&ok);
	if (!ok || index >= m_reader->count())
	{
		QMessageBox::warning(this, QStringLiteral("跳转失败"), QStringLiteral("全局序号无效或超出范围。"));
		return;
	}
	m_frameList->setCurrentRow(static_cast<int>(index));
}

void MainWindow::jumpToSourceIndex()
{
	if (m_reader.get() == nullptr)
	{
		return;
	}
	bool ok = false;
	quint64 sourceIndex = m_sourceIndexEdit->text().trimmed().toULongLong(&ok);
	if (!ok)
	{
		QMessageBox::warning(this, QStringLiteral("跳转失败"), QStringLiteral("sourceIndex 无效。"));
		return;
	}
	try
	{
		quint64 globalIndex = m_reader->findBySourceIndex(sourceIndex);
		m_frameList->setCurrentRow(static_cast<int>(globalIndex));
	}
	catch (const std::exception& ex)
	{
		QMessageBox::warning(this, QStringLiteral("未找到"), QString::fromLocal8Bit(ex.what()));
	}
}

void MainWindow::saveCurrent()
{
	if (m_reader.get() == nullptr || m_currentImage.isNull())
	{
		return;
	}
	QString path = QFileDialog::getSaveFileName(this, QStringLiteral("保存当前图片"), QStringLiteral("frame.jpg"), QStringLiteral("JPEG (*.jpg *.jpeg)"));
	if (path.isEmpty())
	{
		return;
	}
	try
	{
		m_reader->saveJpeg(m_currentIndex, path);
		setStatus(QStringLiteral("已保存：%1").arg(path));
	}
	catch (const std::exception& ex)
	{
		QMessageBox::critical(this, QStringLiteral("保存失败"), QString::fromLocal8Bit(ex.what()));
	}
}

void MainWindow::exportAll()
{
	if (m_reader.get() == nullptr)
	{
		return;
	}
	QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择批量导出目录"));
	if (dir.isEmpty())
	{
		return;
	}
	if (QMessageBox::question(this, QStringLiteral("批量导出"), QStringLiteral("将导出全部图片，文件数量可能很多。是否继续？")) != QMessageBox::Yes)
	{
		return;
	}

	const quint64 totalCount = m_reader->count();
	try
	{
		ExportContext context(this);
		context.timer.start();
		m_exportProgress = new QProgressDialog(QStringLiteral("准备导出..."), QStringLiteral("取消"), 0, static_cast<int>(qMin<quint64>(totalCount, 2147483647ULL)), this);
		m_exportProgress->setWindowTitle(QStringLiteral("批量导出"));
		m_exportProgress->setWindowModality(Qt::ApplicationModal);
		m_exportProgress->setMinimumDuration(0);
		m_exportProgress->setValue(0);

		setActionsEnabled(false);
		QApplication::setOverrideCursor(Qt::WaitCursor);
		int ret = PACK_OK;
#if defined(_WIN32)
		ret = Pack_ExportAll(
			m_reader->nativeHandle(),
			reinterpret_cast<const wchar_t*>(dir.utf16()),
			PACK_NAMING_LEGACY_DIRS,
			&MainWindow::exportCallback,
			&context);
#else
		QByteArray outputDir = dir.toUtf8();
		ret = Pack_ExportAllUtf8(
			m_reader->nativeHandle(),
			outputDir.constData(),
			PACK_NAMING_LEGACY_DIRS,
			&MainWindow::exportCallbackUtf8,
			&context);
#endif
		QApplication::restoreOverrideCursor();
		setActionsEnabled(true);

		const QString summary = formatExportSummary(context, totalCount, dir);
		m_infoText->setPlainText(summary);
		if (context.cancelled)
		{
			setStatus(QStringLiteral("批量导出已取消，已导出 %1 / %2 张").arg(context.exportedFrames).arg(totalCount));
			QMessageBox::information(this, QStringLiteral("导出已取消"), summary);
		}
		else if (ret == PACK_OK)
		{
			setStatus(QStringLiteral("批量导出完成：%1，%2 张，%3").arg(dir).arg(context.exportedFrames).arg(formatBytes(context.exportedBytes)));
			QMessageBox::information(this, QStringLiteral("导出完成"), summary);
		}
		else
		{
			const QString errorText = lastSdkError();
			setStatus(QStringLiteral("批量导出失败：%1").arg(errorText));
			QMessageBox::critical(this, QStringLiteral("导出失败"), summary + QStringLiteral("\n\n错误：") + errorText);
		}
		if (m_exportProgress)
		{
			m_exportProgress->deleteLater();
			m_exportProgress = nullptr;
		}
	}
	catch (const std::exception& ex)
	{
		QApplication::restoreOverrideCursor();
		setActionsEnabled(true);
		if (m_exportProgress)
		{
			m_exportProgress->deleteLater();
			m_exportProgress = nullptr;
		}
		QMessageBox::critical(this, QStringLiteral("导出失败"), QString::fromLocal8Bit(ex.what()));
	}
}

int PACKSDK_CALL MainWindow::exportCallback(
	uint64_t globalIndex,
	uint64_t totalCount,
	const wchar_t* outputPath,
	int result,
	void* userData)
{
	return handleExportCallback(
		globalIndex,
		totalCount,
		outputPath == nullptr ? QString() : QString::fromWCharArray(outputPath),
		result,
		userData);
}

int PACKSDK_CALL MainWindow::exportCallbackUtf8(
	uint64_t globalIndex,
	uint64_t totalCount,
	const char* outputPath,
	int result,
	void* userData)
{
	return handleExportCallback(
		globalIndex,
		totalCount,
		outputPath == nullptr ? QString() : QString::fromUtf8(outputPath),
		result,
		userData);
}

int MainWindow::handleExportCallback(
	uint64_t globalIndex,
	uint64_t totalCount,
	const QString& outputPath,
	int result,
	void* userData)
{
	ExportContext* context = static_cast<ExportContext*>(userData);
	if (context == nullptr || context->window == nullptr || !context->window->m_exportProgress)
	{
		return 0;
	}
	context->lastOutputPath = outputPath;
	context->lastResult = result;
	if (result == PACK_OK)
	{
		context->exportedFrames++;
		QFileInfo fileInfo(context->lastOutputPath);
		if (fileInfo.exists())
		{
			context->exportedBytes += static_cast<quint64>(fileInfo.size());
		}
	}
	else
	{
		context->failedFrames++;
	}

	const qint64 nowMs = context->timer.elapsed();
	const bool finalFrame = (globalIndex + 1 >= totalCount);
	if (finalFrame || result != PACK_OK || nowMs - context->lastUiUpdateMs >= 200)
	{
		context->lastUiUpdateMs = nowMs;
		const double elapsedSec = qMax(0.001, nowMs / 1000.0);
		const double fps = context->exportedFrames / elapsedSec;
		const double mbps = (static_cast<double>(context->exportedBytes) / (1024.0 * 1024.0)) / elapsedSec;
		QProgressDialog* progress = context->window->m_exportProgress;
		progress->setMaximum(static_cast<int>(qMin<uint64_t>(totalCount, 2147483647ULL)));
		progress->setValue(static_cast<int>(qMin<uint64_t>(globalIndex + 1, 2147483647ULL)));
		progress->setLabelText(QStringLiteral("已导出 %1 / %2 张\n耗时 %3 秒，%4 张/秒，%5 MB/秒\n已写入 %6")
			.arg(context->exportedFrames)
			.arg(totalCount)
			.arg(elapsedSec, 0, 'f', 1)
			.arg(fps, 0, 'f', 1)
			.arg(mbps, 0, 'f', 1)
			.arg(formatBytes(context->exportedBytes)));
	}
	QApplication::processEvents();
	if (context->window->m_exportProgress->wasCanceled())
	{
		context->cancelled = true;
		return 1;
	}
	return 0;
}

void MainWindow::verifyDataset()
{
	if (m_reader.get() == nullptr)
	{
		return;
	}
	try
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		PACK_VERIFY_REPORT report = m_reader->verify();
		QApplication::restoreOverrideCursor();
		QMessageBox::information(this, QStringLiteral("校验结果"), QString::fromWCharArray(report.summary));
	}
	catch (const std::exception& ex)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(this, QStringLiteral("校验失败"), QString::fromLocal8Bit(ex.what()));
	}
}

void MainWindow::zoomIn()
{
	m_fitCheck->setChecked(false);
	m_zoom *= 1.25;
	updateImageView();
}

void MainWindow::zoomOut()
{
	m_fitCheck->setChecked(false);
	m_zoom /= 1.25;
	if (m_zoom < 0.05)
	{
		m_zoom = 0.05;
	}
	updateImageView();
}

void MainWindow::actualSize()
{
	m_fitCheck->setChecked(false);
	m_zoom = 1.0;
	updateImageView();
}

void MainWindow::fitToWindowChanged()
{
	updateImageView();
}

QString MainWindow::lastSdkError() const
{
	if (m_reader.get() == nullptr)
	{
		return QString::fromWCharArray(Pack_GetLastError(nullptr));
	}
	return QString::fromWCharArray(Pack_GetLastError(m_reader->nativeHandle()));
}

void MainWindow::setStatus(const QString& text)
{
	statusBar()->showMessage(text);
}
