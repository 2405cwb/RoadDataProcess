#include "hn2dPixWidget.h"
#include "hnImagePainter.h"
#include "addDiseaseDialog.h"
#include "../hnDataTable/hnDBSqliteRoadInfo.h"
#include "../hnDataTable/hnDBSqlite.h" 
#include "hnDataManager.h"
#include "hnProject.h"
#include "hn3DProject.h"
#include "../hnQtCommon/MyCommonMethods.h"
#include "hnDiseaseService.h"
#include "LittleFrameRenderPathBuilder.h"
#include "../TunnelViewerSDK/src/TiledGraphicsView.h"
#include <QMessageBox>
#include <QTime>
#include <QSet>
#include <QEventLoop>
#include <QTimer>
#include <QApplication> 
#include "QMessageBox" 
#include <QTextEdit> 
#include <QElapsedTimer>
#include <QDebug>
#include <algorithm>
#include<QProgressDialog>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QPushButton>
#include <QToolTip>
#include <functional>

class hnLineCameraBoundaryItem : public QGraphicsLineItem
{
public:
	explicit hnLineCameraBoundaryItem(const QColor& color)
	{
		QPen pen(color, 4.0);
		pen.setCosmetic(true);
		setPen(pen);
		setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges | QGraphicsItem::ItemIsFocusable);
		setCursor(Qt::SizeHorCursor);
		setZValue(100000.0);
	}

	void setHorizontalRange(qreal minimum, qreal maximum)
	{
		m_minimum = minimum;
		m_maximum = qMax(minimum, maximum);
	}

	std::function<void()> moved;

protected:
	void keyPressEvent(QKeyEvent* event) override
	{
		if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
		{
			const int step = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
			setPos(pos().x() + (event->key() == Qt::Key_Left ? -step : step), 0.0);
			event->accept();
			return;
		}
		QGraphicsLineItem::keyPressEvent(event);
	}

	QVariant itemChange(GraphicsItemChange change, const QVariant& value) override
	{
		if (change == QGraphicsItem::ItemPositionChange)
		{
			QPointF position = value.toPointF();
			position.setX(qBound(m_minimum, position.x(), m_maximum));
			position.setY(0.0);
			return position;
		}
		if (change == QGraphicsItem::ItemPositionHasChanged && moved)
		{
			moved();
		}
		return QGraphicsLineItem::itemChange(change, value);
	}

private:
	qreal m_minimum = 0.0;
	qreal m_maximum = 0.0;
};
static bool validateLineCameraDiseaseGeometry(const hnRoadDiseaseInfo& disease, QWidget* parent)
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project || !project->isLineCameraProject())
	{
		return true;
	}
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	for (const hn2dRectI& rect : disease.vec2dRect)
	{
		const int xs[] = { rect.p0.x, rect.p1.x, rect.p2.x, rect.p3.x };
		const int pointCount = disease.nDrawType == 3 ? 1 : 4;
		for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
		{
			const int x = xs[pointIndex];
			if (!info.containsPixelX(x))
			{
				QMessageBox::warning(parent, QStringLiteral("病害超出线阵有效区域"),
					QStringLiteral("病害几何中存在横向像素 %1，不在有效区域 [%2, %3] 内，已取消提交。")
						.arg(x).arg(info.leftPixel).arg(info.rightPixel));
				return false;
			}
		}
	}
	return true;
}
hn2dPixWidget::hn2dPixWidget(QWidget *parent)
//: hnBrowsePixWidget(parent)
{
	m_xrSetting = HnXRSettings::getInstance();
	this->m_isDrawingDisease = false;

	this->m_workMode = WorkMode::NO_MODE;

	this->m_frameMode = FrameMode::BIG_FRAME;

	//设置底部帧数前后各加载的帧数
	this->setLoadFrameNum(10);

	m_fontSize = 110;

	m_lineWidth = 20;

	m_widgetType = WIDGET_2D;
	m_lblCoordinates = new QLabel(this);
	m_lblCoordinates->setObjectName("coordLabel");
	m_lblCoordinates->setStyleSheet(
		"#coordLabel{"
		"	background-color: rgba(0,0,0,180);"
		"	color: white !important;"
		"	border: 1px solid #888888;"
		"	border-radius: 8px;"
		"	padding: 8px;"
		"	font-size: 10px;"
		"	font-family: 'Microsoft YaHei','Segoe UI',Arial;"
		"}");
	 
	m_lblCoordinates->hide();

	connect(hnApp::hnDataManager::getDataManager()->getDiseaseService(),
		SIGNAL(diseaseChanged()), this, SLOT(slotDiseaseChanged()));

 
}

void hn2dPixWidget::clearSdkView()
{
	clearLineCameraAreaGuide();
	hn2d3dPixBaseWidget::clearSdkView();
}

void hn2dPixWidget::loadRoadPicture()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	if (!hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		return;
	}

	{
		
		m_highAccuracy = std::make_unique<  HighAccuracyPositioning>(hnApp::hnDataManager::getDataManager()->getCurrentProject());

	}
	//获取翻转配置
	m_isHMirrored = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsHMirrored();
	m_isVMirrored = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored();

	//获取高度比例和宽度比例
	auto setInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	this->m_heightScale = setInfo.dRadioY;
	this->m_widthScale = setInfo.dRadioX;

	//获取图片宽度高度
	this->m_pixWidth = setInfo.picPixelX;
	this->m_pixHeight = setInfo.picPixelY;

	this->m_hnMileVector.clear();
	this->m_pixNameHnMileMap.clear();
	this->m_milePixNameMap.clear();
	this->m_hnMileVector = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector();
	QStringList pixNames;
	for (auto mile : m_hnMileVector)
	{
		if (mile.picturePath.isEmpty())
		{
			continue;
		}
		pixNames.append(mile.picturePath);
		this->m_pixNameHnMileMap.insert(mile.picturePath, mile);
		this->m_milePixNameMap.insert(mile.dEnclMile, mile.picturePath);
	}
	if (pixNames.isEmpty())
	{
		return;
	}
	// RoadDis is the physical distance represented by one 2D road image.
	const double roadImageDistanceMeters = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->_RoadImgDis;
	this->setImageDistanceMeters(roadImageDistanceMeters);
	clearLineCameraAreaGuide();
	this->loadPix(pixNames);
	this->loadSdkVerticalImageSequence(pixNames);
	refreshLineCameraAreaGuide();

	//获取自动化模式数组
	this->m_singleImageLittleFrameRects = this->createSingleImageLittleFrameRect();

	//获取绘制方式
	this->initFrameMode();


#ifdef  ALL_LITTLE_DRAW
	bool isBig = false;
	//20251017改动
	if (m_frameMode == FrameMode::BIG_FRAME || m_frameMode == FrameMode::DESIGN_FACETS)
	{
		m_frameMode = FrameMode::LITTLE_FRAME;
		littleDrawRectType = true;
		isBig = true;
	}
	if (m_frameMode == FrameMode::LITTLE_FRAME && !isBig)
	{
		littleDrawRectType = false;
	}

	//更新所有病害
	QVector<hnRoadDiseaseInfo> allRoadDiseaes = 	getAllRoadDisease();
	for (auto& dis : allRoadDiseaes)
	{
		reCalculateDiseaseSizeAndSave(dis,true);
	}

#endif 
	// Only old automatic diseases with missing dimensions need migration. Do
	// not create a modal dialog or pump the event loop for every normal disease.
	QVector<hnRoadDiseaseInfo> allRoadDiseaes =
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllDiseases();
	QVector<int> migrationIndexes;
	for (int i = 0; i < allRoadDiseaes.size(); ++i)
	{
		const hnRoadDiseaseInfo& disease = allRoadDiseaes.at(i);
		if (disease.nDrawType == 1 && (disease.dLength == 0 || disease.dArea == 0)
			&& !disease.vec2dRect.empty())
		{
			migrationIndexes.append(i);
		}
	}
	if (migrationIndexes.isEmpty())
	{
		m_lineWidth = 20;
		return;
	}

	QProgressDialog progress(QStringLiteral("检测到旧版本病害，自动进行更新，此过程仅一次，耗时较长，请耐心等待"),
		QString(), 0, migrationIndexes.size(), this);
	progress.setWindowTitle(QStringLiteral("处理中..."));
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setMinimumDuration(500);
	progress.setValue(0);
	for (int migrationIndex = 0; migrationIndex < migrationIndexes.size(); ++migrationIndex)
	{
		auto& dis = allRoadDiseaes[migrationIndexes.at(migrationIndex)];
		if (dis.nDrawType == 1)
		{ 
			if (dis.dLength == 0 || dis.dArea == 0)
			{
				std::vector<hn2dRectI>  hn2dRects = dis.vec2dRect;
				if (hn2dRects .size()==0)
				{
					continue;
				}
				QVector<QRect> diseaseRects;

				for (hn2dRectI rect2d : qAsConst(hn2dRects))
				{
					QRect rect = this->hn2dRectToImageQtRect(rect2d);
					diseaseRects.push_back(rect);
				}
				reCalculateOldDiseaseSizeAndSave(diseaseRects, dis);
			} 
		}
		progress.setValue(migrationIndex + 1);
		if ((migrationIndex % 16) == 0)
		{
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}
	progress.setValue(migrationIndexes.size());
	m_lineWidth = 20;
}


void hn2dPixWidget::drawSomeThingOnImage(QImage & image)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject() || !hnApp::hnDataManager::getDataManager()->getCurrentProject())
	{
		return;
	}
	if (this->m_pixNameMap.size() == 0)
	{
		return;
	}

	QElapsedTimer totalTimer;
	QElapsedTimer stepTimer;
	totalTimer.start();

	stepTimer.start();
	this->adjustImage(image);
	const qint64 adjustMs = stepTimer.elapsed();

	stepTimer.restart();
	this->drawDatabaseLoadData(image);
	const qint64 dbMs = stepTimer.elapsed();

	stepTimer.restart();
	this->drawTmpData(image);
	const qint64 tmpMs = stepTimer.elapsed();

	stepTimer.restart();
	this->drawMarkValue(image);
	const qint64 markMs = stepTimer.elapsed();

	stepTimer.restart();
	this->setCurrentHnMile();
	const qint64 mileMs = stepTimer.elapsed();

	const qint64 totalMs = totalTimer.elapsed();
	if (totalMs >= 60 || adjustMs >= 20 || dbMs >= 20 || tmpMs >= 20 || markMs >= 20 || mileMs >= 20)
	{
		#ifdef _DEBUG
		qDebug().noquote() << "[HN_PERF][2DDrawOverlay]"
			<< "totalMs=" << totalMs
			<< "adjustMs=" << adjustMs
			<< "dbMs=" << dbMs
			<< "tmpMs=" << tmpMs
			<< "markMs=" << markMs
			<< "mileMs=" << mileMs
			<< "bottomFrame=" << m_buttomFrameIdx
			<< "currentFrameNum=" << m_currentWidgetFrameNum
			<< "visiblePix=" << m_currentWidgetPixNames.size()
			<< "imageSize=" << QString("%1x%2").arg(image.width()).arg(image.height());
		#endif
	}
}
void hn2dPixWidget::mousePressEvent(QMouseEvent * event)
{
	m_lblCoordinates->hide();
	bool isvalid = isValidArea(event);
	if (!isvalid)
	{
		return;
	}
	if (event->button() == Qt::LeftButton)
	{
		// After canceling the disease dialog, drawing restarts only on the next left press.
		m_waitLittleFrameLeftPressAfterCancel = false;
	}

	// 使用右键进行删除自动化模式病害，所以这里右键取消话病害功能不使用
#if 0
	//右键取消画病害
	if (event->button() == Qt::RightButton && this->m_workMode == WorkMode::ADD_MODE)
	{
		this->slot_cancelDrawDiseases();
	}
#endif

	if (event->button() == Qt::RightButton)
	{
		const bool rightClickDeletesLittleFrame =
			this->m_frameMode == FrameMode::LITTLE_FRAME && !this->m_isDrawingDisease;
		if (!rightClickDeletesLittleFrame)
		{
			selectDisease(event->pos());
		}
	}
	
 
	//人工模式模式 添加病害 鼠标左键

	if (this->m_frameMode == FrameMode::BIG_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:
				// 人工模式添加病害
				this->bigFrameAddDisease(event->pos());
				break;
			case hnWorkMode::DELETE_MODE:
				this->commonDeleteDisease(event->pos(), FrameMode::BIG_FRAME);
				break;
			case hnWorkMode::EDIT_MODE:
				this->bigFrameEditProcess(event->pos());
				break;
			case hnWorkMode::MOVE:
				break;
			case hnWorkMode::MERGE:
				this->bigFrameMergeDiseases(event->pos());
				break;
			case hnWorkMode::GET_MILE:
				break;
			case hnWorkMode::ADD_CTRL_POINT:
				break;
			default:
				break;
			}
		}
		if (event->button() == Qt::RightButton)
		{
			if (m_isDrawingDisease)
			{
				//取消绘制病害
				this->m_isDrawingDisease = false;
				this->m_isAllowDrawPix = false;
				this->m_isAllowLinked = true;
				this->update();
			}
		}

	}  

	if (this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:
			{
				if (this->addLineDiseType)  //左键连续点击模式添加线状病害
				{
					this->m_isDrawingDisease = true;
					this->m_isAllowDrawPix = false;
				 

															//修改是否联动
					this->m_isAllowLinked = false;
					if (!this->m_isEndAddPoint)
					{
						m_diseaseAddPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseAddPoint.pixName);

						//记录单张图片的点到折线数组中
						m_littleSingleImagePoints.append(m_diseaseAddPoint);
						//转化单张图片的点数组到拼接图片的点数组
						QPoint bigImagePoint = this->singleImagePointToBigImagePoint(m_diseaseAddPoint.pixPoint, m_diseaseAddPoint.pixName);
						m_litteBigImagePoints.append(bigImagePoint);

						m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;
						m_tmpPaintLineDiseasePoints.append(m_diseaseAddPoint);
						m_tmpLastPaintLineDiseasePoints.append(m_diseaseAddPoint);
						this->update();
					}

				}
				else  //自动跟踪鼠标移动轨迹模式添加病害
				{
					if (this->m_isDrawingDisease)
					{
						//自动化模式病害整个流程
						if (!this->littleFrameProcess())
						{
							this->m_isDrawingDisease = false;
							this->m_isAllowDrawPix = false;
							this->m_isAllowLinked = true;
							return;
						}
					}
				 
					this->m_isDrawingDisease = !this->m_isDrawingDisease;
					this->m_isAllowDrawPix = !this->m_isDrawingDisease;
					//修改是否联动
					m_isAllowLinked = !m_isAllowLinked;
					//清空自动化模式病害数组
					if (this->m_isDrawingDisease)
					{
						resetLittleFrameDrawState();
					}
					QPoint pos = event->pos();
					//记录开始点
					m_diseaseStartPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseStartPoint.pixName);
					//记录结束点
					m_diseaseEndPoint = m_diseaseStartPoint;
				}
			}
				break;
			case hnWorkMode::DELETE_MODE:
				this->commonDeleteDisease(event->pos(), FrameMode::LITTLE_FRAME);

				break;
			case hnWorkMode::EDIT_MODE:
				this->littleFrameEditDisease(event->pos());
				break;
			case hnWorkMode::MOVE:
				break;
			case hnWorkMode::MERGE:
				this->littleFrameMergeDiseases(event->pos());
				break;
			case hnWorkMode::GET_MILE:
				break;
			case hnWorkMode::ADD_CTRL_POINT:
				break;
			default:
				break;
			}
		}
		if (event->button() == Qt::RightButton)
		{
			if (m_isDrawingDisease)
			{
				if (this->addLineDiseType)
				{
					this->m_isEndAddPoint = true;
					this->m_tempPoints.clear();
					 

					if (m_littleSingleImagePoints.size() > 1)
					{
						QVector<pixImagePoint> tmpLittleSingleImagePoints = m_littleSingleImagePoints;

						for (int i = 1; i < m_littleSingleImagePoints.size(); ++i)
						{
							QPoint startPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i - 1].pixPoint, m_littleSingleImagePoints[i - 1].pixName);
							QPoint endPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i].pixPoint, m_littleSingleImagePoints[i].pixName);
							QLine line(startPoint, endPoint);

							//获取图片名字
							QString startName = m_littleSingleImagePoints[i - 1].pixName;
							QString endName = m_littleSingleImagePoints[i].pixName;

							//根据图片名字获取帧序号
							int startIdx = m_pixNameMap.key(startName, -1);
							int endIdx = m_pixNameMap.key(endName, -1);

							//如果结束的帧号小，就交换一下
							if (qMin(startIdx, endIdx) == endIdx)
							{
								qSwap(startIdx, endIdx);
							}

							//左键连续点击添加病害模式下，若相邻点击点之间存在跨多个单张图像时，需额外补充点以生成完整的小方框
							if ((endIdx - startIdx) > 1)
							{
								for (int i = startIdx + 1; i < endIdx; i++)
								{
									QString pixName = m_pixNameMap.value(i, "");
									if (!pixName.isEmpty())
									{
										pixImagePoint tmpInsertPoint;
										tmpInsertPoint.pixName = pixName;
										tmpInsertPoint.pixPoint = QPoint(100, 100);
										tmpLittleSingleImagePoints.append(tmpInsertPoint);
									}
								}
							}

							//生成多段折线病害自动化模式
							this->m_currentLittleFrameRects = this->createLittleFrameRects(tmpLittleSingleImagePoints);

							//判断多段折线与当前自动化模式矩形数组 相交的矩形数组
							hn2d3dCoordinates tool;
							QVector<QRect> tmpDiseaseRects = tool.crossLineOver(line, this->m_currentLittleFrameRects);

							for (auto tmpDiseaseRect : tmpDiseaseRects)
							{
								if (!this->m_tmpLittleFrameDiseaseRects.contains(tmpDiseaseRect))
								{
									this->m_tmpLittleFrameDiseaseRects.append(tmpDiseaseRect);
								}
							}
						}

					}

					if (this->m_isDrawingDisease)
					{
						//自动化模式病害整个流程
						if (!this->littleFrameProcess())
						{
							this->m_isDrawingDisease = false;
							this->m_isAllowDrawPix = false;
							this->m_isAllowLinked = true;
							this->m_isEndAddPoint = false;

							//清空自动化模式病害数组
							m_diseaseAddPoint.pixPoint = QPoint(-1, -1);
							m_littleSingleImagePoints.clear();
							m_litteBigImagePoints.clear();
							m_tmpLittleFrameDiseaseRects.clear();

							return;
						}
						this->m_isDrawingDisease = !this->m_isDrawingDisease;
						this->m_isAllowDrawPix = !this->m_isDrawingDisease;
						//修改是否联动
						m_isAllowLinked = !m_isAllowLinked;
						//清空自动化模式病害数组
						m_diseaseAddPoint.pixPoint = QPoint(-1, -1);
						m_littleSingleImagePoints.clear();
						m_litteBigImagePoints.clear();
						m_tmpLittleFrameDiseaseRects.clear();

						this->m_isEndAddPoint = false;
					}

					 
				}
				else
				{
					this->slot_cancelDrawDiseases();
					event->accept();
					return;
				}
			}
			else
			{
				this->m_isRightDeleteMouseDown = true;
				m_RightDeleteMousePoint = event->pos();
				

				++m_pendingRightClcikDeleteSerial;
				m_pendingRightClickDeletePoint = QPoint(-1, -1);
				this->littleFrameRightButtonDragDelete(event->pos());



			} 
		} 
	}   


	if (m_frameMode == DESIGN_FACETS)
	{
		switch (m_workMode)
		{
		case hnWorkMode::NO_MODE:
			break;
		case hnWorkMode::ADD_MODE:
			// 人工模式添加病害
			this->bigFrameAddDisease(event->pos());
			break;
		case hnWorkMode::DELETE_MODE:
			break;
		case hnWorkMode::EDIT_MODE:
			break;
		case hnWorkMode::MOVE:
			break;
		case hnWorkMode::MERGE:
			break;
		case hnWorkMode::GET_MILE:
			break;
		case hnWorkMode::ADD_CTRL_POINT:
			break;
		default:
			break;
		}
	}
	if (m_frameMode == DESIGN_LINE)
	{
	
		switch (m_workMode)
		{
		case hnWorkMode::NO_MODE:
			break;
		case hnWorkMode::ADD_MODE:
			this->addLineDisease(event->pos());
			break;
		case hnWorkMode::DELETE_MODE:
			break;
		case hnWorkMode::EDIT_MODE:
			break;
		case hnWorkMode::MOVE:
			break;
		case hnWorkMode::MERGE:
			break;
		case hnWorkMode::GET_MILE:
			break;
		case hnWorkMode::ADD_CTRL_POINT:
			break;
		default:
			break;
		}
	}

	bool isDesignMode = m_frameMode == DESIGN_FACETS || DESIGN_LINE == m_frameMode;

	if (isDesignMode)
	{
		switch (m_workMode)
		{
		case hnWorkMode::NO_MODE:
			break;
		case hnWorkMode::ADD_MODE:
			break;
		case hnWorkMode::DELETE_MODE:
			this->commonDeleteDisease(event->pos(), FrameMode::DESIGN_FACETS);
			this->commonDeleteDisease(event->pos(), FrameMode::DESIGN_LINE);
			break;
		case hnWorkMode::EDIT_MODE:
			this->bigFrameEditProcess(event->pos());
			this->commonEditDisease(event->pos(), hnFrameMode::DESIGN_LINE);
			break;
		case hnWorkMode::MOVE:
			break;
		case hnWorkMode::MERGE:
			this->bigFrameMergeDiseases(event->pos());
			this->mergeLineDisease(event->pos());
			break;
		case hnWorkMode::GET_MILE:
			break;
		case hnWorkMode::ADD_CTRL_POINT:
			break;
		default:
			break;
		}
	}

	//获取点击点的编码器里程，用于二维三维里程差值矫正
	if (m_workMode == WorkMode::GET_MILE)
	{
		//获取点击点的编码器里程
		m_encoderMile = caculateEncoderMileByScreenPoint(event->pos());
		//转为大imagePoint 保存下来
		m_seclectPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_seclectPoint.pixName);
	}
	 
  
	hn2d3dPixBaseWidget::mousePressEvent(event);
}


void hn2dPixWidget::mouseReleaseEvent(QMouseEvent * event)
{

	// 鼠标右键释放
	if (event->button() == Qt::MouseButton::RightButton)
	{
		this->m_isRightDeleteMouseDown = false;
		m_RightDeleteMousePoint = QPoint(-1, -1);			// 右键释放，将坐标设置为无效值
	}
}

void hn2dPixWidget::mouseMoveEvent(QMouseEvent * event)
{ 
	currentMousePos = event->screenPos().toPoint();				// 记录鼠标在屏幕的位置

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	if (m_pixNameMap.empty())
	{
		return;
	}
	// 程序翻页后自动移动鼠标，这一次 mouseMove 不允许参与绘制
	if (ignoreMouseMoveAfterAutoCursorMove(event))
	{
		return;
	}


	double dDiseaseLon,  dDiseaseLat, dDiseaseH;
	//获取鼠标位置
	if (m_xrSetting->showGpsInfo&&!this->m_isDrawingDisease)
	{
		int curPosX = 0;
		int curPosY = 0;
		int pictureW = 0; 
		int pictureH = 0;
		double curMile = 0;
		QPoint pos = event->pos();
		QString pixName;
		QPoint bigImagePoint = this->screenPointToBigImagePoint(pos);
		hnMile currentPointMile = this->getHnMileFromPoint(bigImagePoint);
		curMile = currentPointMile.dTrueMile;
		QPoint singleImagePoint = this->screenToSingleImagePoint(pos, pixName);
		curPosX = singleImagePoint.x();
		curPosY = singleImagePoint.y();
		pictureW = m_pixWidth;
		pictureH = m_pixHeight;
		if (m_isHMirrored)
		{
			curPosX = pictureW - curPosX;
		}
		if (m_isVMirrored)
		{
			curPosY = pictureH - curPosY;
		} 
		m_highAccuracy->getHighAccPosition(m_xrSetting->gpsFormat, m_xrSetting->equipType, curMile,curPosX,curPosY , dDiseaseLon, dDiseaseLat, dDiseaseH); 
		m_latitude    = QString::number(dDiseaseLat, 'f', 6);
		m_longitude = QString::number(dDiseaseLon, 'f', 6);
		m_centerH    = QString::number(dDiseaseH, 'f', 2);
		bool canShow = true;
		if (dDiseaseLat <=1)
		{
			canShow = false;
		}
		m_lblCoordinates->setText(
			QStringLiteral("坐标：(%1,%2)\n"
				"经度 %3°E\n"
				"纬度 %4°N\n"
				"高程 %5 M"
			).arg(curPosX).arg(curPosY).arg(m_longitude).arg(m_latitude).arg(m_centerH)
		);
		m_lblCoordinates->adjustSize();

		int labelWidth = m_lblCoordinates->width();
		int labelHeight = m_lblCoordinates->height();

		const int offsetX = 10;
		const int offsetY = 15;

		int targetX = pos.x() + offsetX;
		int targetY = pos.y() + offsetY;
		int widgetWidth = this->width();
		int widgetHeight = this->height();
		if (targetX + labelHeight >widgetWidth )
		{
			targetX = pos.x() - labelWidth - offsetX;
		}
		if (targetY + labelHeight >widgetHeight)
		{
			targetY = pos.y() - labelHeight - offsetY;
		}
		if (targetX< 0 )
		{
			targetX = 0;
		}
		if (targetY<0)
		{
			targetY = 0;
		}
		if (canShow)
		{
			m_lblCoordinates->move(targetX, targetY);
			m_lblCoordinates->show();
			m_lblCoordinates->raise();
		}
	
	}

		// After canceling the disease dialog, drawing restarts only on the next left press.
	if (this->m_isDrawingDisease && !m_waitLittleFrameLeftPressAfterCancel && this->addLineDiseType && !m_tmpPaintLineDiseasePoints.empty())
	{
		QPoint pos = event->pos();
		pixImagePoint endPoint;
		endPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), endPoint.pixName);

		m_tempPoints.clear();
		m_tempPoints.push_back(m_tmpPaintLineDiseasePoints.back());

		if (!endPoint.pixName.isEmpty() && endPoint.pixPoint.rx() > 0 && endPoint.pixPoint.ry() > 0)
		{
			m_tempPoints.push_back(endPoint);
		}

		this->update();

	}

		// After canceling the disease dialog, drawing restarts only on the next left press.
	if (this->m_isDrawingDisease && !m_waitLittleFrameLeftPressAfterCancel && !this->addLineDiseType)
	{
		//如果正在画临时病害，就不允许画图片
		m_isAllowDrawPix = false;
		//记录结束点
		m_diseaseEndPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseEndPoint.pixName);
		//记录单张图片的点到折线数组中
		if (this->m_frameMode == FrameMode::LITTLE_FRAME && this->m_workMode == WorkMode::ADD_MODE)
		{
			this->setCursor(Qt::CrossCursor);

			if (!this->littleDrawRectType)
			{
				m_littleSingleImagePoints.append(m_diseaseEndPoint);

				m_litteBigImagePoints.clear();
				for (auto point : qAsConst(m_littleSingleImagePoints))
				{
					QPoint bigImagePoint = singleImagePointToBigImagePoint(point.pixPoint, point.pixName);
					m_litteBigImagePoints.append(bigImagePoint);
				}
			}
		}

		// D-rectangle mode uses only the current drag rectangle, not historical line points.
		if (!this->littleDrawRectType)
		{
			m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;
			m_tmpPaintLineDiseasePoints.append(m_diseaseEndPoint);
			m_tmpLastPaintLineDiseasePoints.append(m_diseaseEndPoint);
		}
		this->update();
	}

#if 0
	// 只删除鼠标按下位置的自动化模式
	// 右键删除自动化模式病害中的某个自动化模式。判断坐标是否满足要求，从而不会重复处理
	if (this->m_isRightDeleteMouseDown&&this->m_RightDeleteMousePoint.x() >= 0 && this->m_RightDeleteMousePoint.y() >= 0)
	{
		this->littleFrameRightButtonDragDelete(m_RightDeleteMousePoint);
		m_RightDeleteMousePoint.setX(-1);
		m_RightDeleteMousePoint.setY(-1);
	}
#else
	// 删除鼠标移动路径上的自动化模式
	if (this->m_isRightDeleteMouseDown)
	{
		++m_pendingRightClcikDeleteSerial;
		m_pendingRightClickDeletePoint = QPoint(-1, -1);
		this->littleFrameRightButtonDragDelete(event->pos());
	}

#endif



	//更新原始比例窗口
	QImage originalImage = this->getOriginalImage(event->pos(), m_tmpPixImageWithoutDisease, m_originalWidgetWidth, m_originalWidgetHeight);
	sig_mousePosImageChanged(originalImage);

	//获取状态栏所需要的信息，并发送信号
	if (!m_sdkImageView)
	{
		QString statusInfo = generateStatusInfo(event->pos());
		emit this->signal_statusInfoChanged(statusInfo);
	}
	QWidget::mouseMoveEvent(event);
}



//void hn2dPixWidget::wheelEvent(QWheelEvent * event)
//{
//	//在滚轮滚动的时候，允许画病害图片
//	this->m_isAllowDrawPix = true;
//	// 延时发送事件，避免出问题
//	QTimer::singleShot(5, [this, event]() {
//		//触发鼠标移动事件
//		QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseMove, this->mapFromGlobal(QCursor().pos()),
//			Qt::NoButton, Qt::NoButton, Qt::NoModifier);
//		QApplication::sendEvent(this, mouseEvent);
//		delete mouseEvent;
//
//	});
//	bool up = event->delta() > 0 ? true : false;
//	if (this->m_workMode == WorkMode::ADD_MODE&&
//		this->m_isDrawingDisease
//		&& this->m_frameMode == FrameMode::LITTLE_FRAME)
//	{
//		//小框的绘制临时停止绘制策略
//		isSuspended = true;
//
//	}
//
//
//	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
//	{
//		return;
//	}
//}


void hn2dPixWidget::wheelEvent(QWheelEvent * event)
{
	this->m_isAllowDrawPix = true;

	const bool up = event->delta() > 0;

	if (isDrawingLittleFrameDisease())
	{
		scheduleMoveCursorToBestContinuePointAfterBrowse(up, true);
	}
	else if (this->m_isDrawingDisease &&
		this->m_frameMode == FrameMode::LITTLE_FRAME &&
		this->littleDrawRectType)
	{
		this->commitCurrentLittleRectDrawSelection();
		QTimer::singleShot(20, this, [this]()
		{
			this->update();
		});
	}
	event->ignore();
}



void hn2dPixWidget::leaveEvent(QEvent * event)
{
	m_lblCoordinates->hide();
	QWidget::leaveEvent(event);
}

void hn2dPixWidget::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_G)
	{
		bool showGps = m_xrSetting->showGpsInfo;
		if (showGps)
		{
			QDialog dilalog(this);
			dilalog.setWindowTitle(QStringLiteral("坐标信息"));

			QTextEdit * infoTextEdit = new QTextEdit(this);

			infoTextEdit->setFrameStyle(QFrame::NoFrame);

			infoTextEdit->setText(m_longitude + "\n" + m_latitude + "\n" + m_centerH);

			QVBoxLayout * layout = new QVBoxLayout(&dilalog);
			layout->addWidget(infoTextEdit);
			layout->setContentsMargins(10, 10, 10, 10);
			dilalog.setLayout(layout);
			dilalog.exec();
		}
	}

	if (event->key() == Qt::Key_Delete)
	{
		// qDebug() << QStringLiteral("按下了删除键");
		QPoint screenPoint = QCursor::pos();
		QPoint widgetPoint = this->mapFromGlobal(screenPoint);
		if (m_frameMode == LITTLE_FRAME)
		{

			for (auto& dis: m_seclectedDiseases)
			{
				hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(dis);

			}
			m_seclectedDiseases.clear();
			
			
			// 按下 删除键 时，获取鼠标位置，删除病害
			//this->littleFrameRightButtonDragDelete(widgetPoint);
		}
	}
	if (event->key() == Qt::Key_R && this->m_frameMode == LITTLE_FRAME && this->m_workMode == ADD_MODE)
	{
		const bool enableRectMode = !this->littleDrawRectType;
		resetSdkDiseaseDrawingState();
		this->littleDrawRectType = enableRectMode;
		this->addLineDiseType = false;
		event->accept();
		return;
	}

	// 按下 N键 时，结束左键连续点击添加线状病害
	if (event->key() == Qt::Key_N && this->addLineDiseType)
	{
		if (hasSdkImageView())
		{
			if (this->m_isDrawingDisease)
			{
				finishSdkLineLittleFrameDisease();
			}
			event->accept();
			return;
		}

		this->m_isEndAddPoint = true;
		this->m_tempPoints.clear();
		//this->m_isAllowDrawDashLine = false;

		if (m_littleSingleImagePoints.size() > 1)
		{
			QVector<pixImagePoint> tmpLittleSingleImagePoints = m_littleSingleImagePoints;

			for (int i = 1; i < m_littleSingleImagePoints.size(); ++i)
			{
				QPoint startPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i - 1].pixPoint, m_littleSingleImagePoints[i - 1].pixName);
				QPoint endPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i].pixPoint, m_littleSingleImagePoints[i].pixName);
				QLine line(startPoint, endPoint);

				//获取图片名字
				QString startName = m_littleSingleImagePoints[i - 1].pixName;
				QString endName = m_littleSingleImagePoints[i].pixName;

				//根据图片名字获取帧序号
				int startIdx = m_pixNameMap.key(startName, -1);
				int endIdx = m_pixNameMap.key(endName, -1);
 
				//如果结束的帧号小，就交换一下
				if (qMin(startIdx, endIdx) == endIdx)
				{
					qSwap(startIdx, endIdx);
				}

				//左键连续点击添加病害模式下，若相邻点击点之间存在跨多个单张图像时，需额外补充点以生成完整的小方框
				if ((endIdx - startIdx) > 1)
				{
					for (int i = startIdx + 1; i < endIdx; i++)
					{
						QString pixName = m_pixNameMap.value(i, "");
						if (!pixName.isEmpty())
						{
							pixImagePoint tmpInsertPoint;
							tmpInsertPoint.pixName = pixName;
							tmpInsertPoint.pixPoint = QPoint(100, 100);
							tmpLittleSingleImagePoints.append(tmpInsertPoint);
						}
					}
				}

				//生成多段折线病害自动化模式
				this->m_currentLittleFrameRects = this->createLittleFrameRects(tmpLittleSingleImagePoints);

				//判断多段折线与当前自动化模式矩形数组 相交的矩形数组
				hn2d3dCoordinates tool;
				QVector<QRect> tmpDiseaseRects = tool.crossLineOver(line, this->m_currentLittleFrameRects);

				for (auto tmpDiseaseRect : tmpDiseaseRects)
				{
					if (!this->m_tmpLittleFrameDiseaseRects.contains(tmpDiseaseRect))
					{
						this->m_tmpLittleFrameDiseaseRects.append(tmpDiseaseRect);
					}
				}
			}
		 
		}

		if (this->m_isDrawingDisease)
		{
			//自动化模式病害整个流程
			if (!this->littleFrameProcess())
			{
				this->m_isDrawingDisease = false;
				this->m_isAllowDrawPix = false;
				this->m_isAllowLinked = true;
				this->m_isEndAddPoint = false;

				//清空自动化模式病害数组
				m_diseaseAddPoint.pixPoint = QPoint(-1, -1);
				m_littleSingleImagePoints.clear();
				m_litteBigImagePoints.clear();
				m_tmpLittleFrameDiseaseRects.clear();

				return;
			}
			this->m_isDrawingDisease = !this->m_isDrawingDisease;
			this->m_isAllowDrawPix = !this->m_isDrawingDisease;
			//修改是否联动
			m_isAllowLinked = !m_isAllowLinked;
			//清空自动化模式病害数组
			m_diseaseAddPoint.pixPoint = QPoint(-1, -1);
			m_littleSingleImagePoints.clear();
			m_litteBigImagePoints.clear();
			m_tmpLittleFrameDiseaseRects.clear();

			this->m_isEndAddPoint = false;
		} 
	}

	// 上传其他没有处理的消息
	hn2d3dPixBaseWidget::keyPressEvent(event);
	//QWidget::keyPressEvent(event);//
}




bool hn2dPixWidget::isTmpDiseaseRoadTypeValid(const int & frameType)
{

	QSet<QString> pixNames;

	//人工模式找图片名字
	if (frameType == 0)
	{
		//获取图片名字
		QString beginName = m_diseaseStartPoint.pixName;
		QString endName = m_diseaseEndPoint.pixName;

		//根据图片名字获取帧序号
		int beginIdx = m_pixNameMap.key(beginName, -1);
		int endIdx = m_pixNameMap.key(endName, -1);
		if (beginIdx == -1 && endIdx == -1)
		{
			return false;
		}
		//如果结束的帧号小，就交换一下
		if (qMin(beginIdx, endIdx) == endIdx)
		{
			qSwap(beginIdx, endIdx);
		}

		//把所有帧号的图片找出来	
		for (int i = beginIdx; i <= endIdx; i++)
		{
			QString pixName = m_pixNameMap.value(i, "");
			if (!pixName.isEmpty())
			{
				pixNames.insert(pixName);
			}
		}
	}
	//自动化模式找图片名字
	else if (1 == frameType)
	{
		for (auto rect : qAsConst(m_tmpLittleFrameDiseaseRects))
		{
			QPoint p = rect.topLeft();
			QString pixName;
			this->bigImagePointToSingleImagePoint(p, &pixName);
			if (!pixName.isEmpty())
			{
				pixNames.insert(pixName);
			}
		}
	}
	//线状病害
	else if (3 == frameType)
	{
		for (auto point : qAsConst(m_tmpLineDiseasePoints))
		{
			QPoint p = point.pixPoint;
			QString pixName;
			this->bigImagePointToSingleImagePoint(p, &pixName);
			if (!pixName.isEmpty())
			{
				pixNames.insert(pixName);
			}
		}
	}
	else
	{
		return true;
	}

	QVector<hnMile> hnMiles;
	hnMile dftMile;
	dftMile.dEnclMile = -1;
	//找到所有的hnMile
	for (auto pixName : qAsConst(pixNames))
	{
		hnMile mile = m_pixNameHnMileMap.value(pixName, dftMile);
		if (mile.dEnclMile != -1)
		{
			hnMiles.append(mile);
		}
	}

	//遍历所有hnMile，如果有两种及以上路面标准或者路面类型,则判定为无效病害
	QSet<HnProjectEnums::StandardParmTypeEnum> roadLevels;
	QSet<hnCommon::ROAD_SURFACE_TYPE> roadTypes;
	for (auto mile : qAsConst(hnMiles))
	{
		roadLevels.insert(mile.roadStandard);
		roadTypes.insert(mile.roadType);
	}

	if (roadLevels.size() >= 2 || roadTypes.size() >= 2)
	{
		return false;
	}

	QString invalidReason;
	if (!isTmpDiseaseRoadMarkRangeValid(frameType, &invalidReason))
	{
		return false;
	}

	return true;
}


QVector<QPoint> hn2dPixWidget::createBrokenLinePoints(hnRoadDiseaseInfo & disease)
{
	QVector<QPoint> result;
	for (auto rect : qAsConst(disease.vec2dRect))
	{
		result.push_back(hn2dPointToImageQtPoint(rect.p0));
	}

	return result;
}

bool hn2dPixWidget::drawBigFrameProcess()
{
	QRect diseaseRect;
	if (hasSdkImageView())
	{
		QVector<SdkSingleImageRect> sdkRects;
		if (!currentSdkBigFrameSingleRects(sdkRects))
		{
			resetSdkDiseaseDrawingState(true, false);
			return false;
		}

		// SDK commit uses the first single-image rect only as legacy dialog input; real geometry is stored from SDK selections.
		diseaseRect = sdkRects.first().singleRect.normalized();
	}
	else
	{
		QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
		QPoint bigImageEnd = this->singleImagePointToBigImagePoint(m_diseaseEndPoint.pixPoint, m_diseaseEndPoint.pixName);
		diseaseRect = QRect(bigImageStart, bigImageEnd).normalized();
	}

	hnMile mile = m_firstHnMile;

	QStringList diseaseTypeList;
	QList<QPair<QString, QString>> diseaseNameAndKey;
	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	//如果病害无效，则取消画病害
	if (!this->isTmpDiseaseRoadTypeValid(0))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		return false;
	}
	if (this->m_firstHnMile.roadWidth == -1)
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害未获取到有效路面标准或者路面宽度，病害无效，取消绘制，请适当移动图像重新绘制！"),
			QString::fromLocal8Bit("确定"));
		return false;
	}
	if (diseaseSetInfos.isEmpty())
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("当前位置没有匹配的病害类型，请检查路面标准、路面材质和作业模式设置。"),
			QString::fromLocal8Bit("确定"));
		resetSdkDiseaseDrawingState(true, false);
		return false;
	}


	//弹出添加病害窗口
	QString diseaseTypeName;
	QString diseaseMark;
	if (!selectDiseaseTypeForDrawing(diseaseSetInfos, diseaseTypeName, diseaseMark))
	{
		resetSdkDiseaseDrawingState(true, false);
		return false;
	}


	QString diseaseTableName;

	hnDiseaseSetInfo selectDiseaseSetInfo;
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		if (QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName) == diseaseTypeName)
		{
			selectDiseaseSetInfo = diseseSetInfo;
			diseaseTableName = QString::fromLocal8Bit(diseseSetInfo.strDBTableName);
		}
	}
	auto diseaseInfo = this->caculateBigFrameDiseaseAttribute(diseaseRect, selectDiseaseSetInfo,diseaseMark);

	if (!this->isTmpDiseaseAreaValid(diseaseInfo))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害面积与规范不符，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		return false;
	}
	 
	if (false == diseaseInfo.vec3dRect.empty() && true == m_isOPenDepthCaculate && nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算人工模式病害深度信息
		if (!this->caculateBigFrameDiseaseDepth(diseaseInfo))
		{
			return false;
		}
	}


	//写入数据库
	//for (int i = 0; i < 2000; i++)
	{

		diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());
		

		if (!validateLineCameraDiseaseGeometry(diseaseInfo, this))
		{
			return false;
		}

		if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo))
		{
			return false;
		}
		rememberLastSdkAddedDisease(diseaseInfo);
 
	} 

	return true;
}

void hn2dPixWidget::drawBigFrameDisease(const vector<hnRoadDiseaseInfo>& diseases, QImage &image, int drawType)
{
	for (auto disease : diseases)
	{
		if (disease.vec2dRect.empty() || disease.nDrawType != drawType)
		{
			continue;
		}

		hn2dRectI hnRect = disease.vec2dRect.at(0);
		QRect diseaseRect = hn2dRectToImageQtRect(hnRect);

		const bool selected = selectedDiseaseId == disease.nID;
		const bool mergeSelected = isSeclectedMergeDisease(disease);

		QColor rectColor = m_diseaseDrawStyle.bigFrameRectColor;
		Qt::PenStyle penStyle = Qt::SolidLine;

		if (selected)
		{
			rectColor = m_diseaseDrawStyle.selectedRectColor;
			penStyle = Qt::DashDotDotLine;
		}
		else if (mergeSelected)
		{
			rectColor = m_diseaseDrawStyle.mergedRectColor;
		}

		const int rectWidth = selected
			? m_diseaseDrawStyle.selectedRectWidth
			: m_diseaseDrawStyle.bigFrameRectWidth;

		const int fontSize = selected
			? m_diseaseDrawStyle.selectedLabelFontSize
			: m_diseaseDrawStyle.normalLabelFontSize;

		const QString diseaseInfo = buildFrameDiseaseLabel(
			disease,
			selected,
			true,
			false);

		drawDiseaseRectWithCallout(
			image,
			diseaseRect,
			diseaseInfo,
			diseaseImagePixels(image,rectWidth),
			//rectWidth,
			rectColor,
			penStyle,
		 
			diseaseImagePixels(image, fontSize)
		);
	}
}

void hn2dPixWidget::bigFrameAddDisease(const QPoint & mousePoint)
{
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject() && !hnApp::hnDataManager::getDataManager()->getCurrentProject()->ensureInitialSurfaceMaterial(this)) return;
	//检测点是否有效
	if (!this->isValidPoint(mousePoint))
	{
		this->m_isDrawingDisease = false;
		this->m_isAllowDrawPix = false;
		this->m_isAllowLinked = true;
		return;
	}

	if (this->m_isDrawingDisease)
	{
		if (!this->drawBigFrameProcess())
		{ 
			this->m_isDrawingDisease = false;
			this->m_isAllowDrawPix = false;
			this->m_isAllowLinked = true;
			return;
		}
	}
	//是否画病害取非
	this->m_isDrawingDisease = !this->m_isDrawingDisease;

	//修改是否绘制临时内容
	this->m_isAllowDrawPix = !this->m_isDrawingDisease;

	//修改是否允许联动
	m_isAllowLinked = !m_isAllowLinked;

	//记录开始点
	m_diseaseStartPoint.pixPoint = this->screenToSingleImagePoint(mousePoint, m_diseaseStartPoint.pixName);

	//计算hnMile  根据开始点来算
	QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);
	this->m_firstHnMile = this->getHnMileFromPoint(bigImagePoint);
}


void hn2dPixWidget::bigFrameEditProcess(const QPoint & mousePoint)
{
auto disease = 	getMousePosDisease(mousePoint);
if (!disease.isValid())
{
	return;
}

m_seclectedDiseases.clear();
m_seclectedDiseases.append(disease);
hn2dRectI  hnRect = disease.vec2dRect.at(0);
QRect diseaseRect = this->hn2dRectToImageQtRect(hnRect);
this->editDisease(disease, diseaseRect.topLeft());
m_seclectedDiseases.clear();
return;
}

void hn2dPixWidget::bigFrameMergeDiseases(const QPoint & screenPoint)
{
	//获取选中的病害，添加到数组中
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(screenPoint, disease, hnFrameMode::BIG_FRAME))
		{
			m_seclectedDiseases.append(disease);
			this->update();
			break;
		}
	}

	//如果等于两个,就合并两个，放到数据库中，删除原来的两个
	if (m_seclectedDiseases.size() == 2)
	{
		//第一个病害的大image QRect
		hn2dRectI  hnRect1 = m_seclectedDiseases.at(0).vec2dRect.at(0);
		QRect diseaseRect1 = hn2dRectToImageQtRect(hnRect1);
		//第二个病害的大image QRect
		hn2dRectI  hnRect2 = m_seclectedDiseases.at(1).vec2dRect.at(0);
		QRect diseaseRect2 = hn2dRectToImageQtRect(hnRect2);
		//新的大image QRect
		QRect newDiseaseRect = diseaseRect1.united(diseaseRect2);

		//新病害
		auto newDisease = m_seclectedDiseases.at(0);

		//新病害有些属性继承了合并病害的第一个，有很多要重新计算

		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		newDisease.dWidth = newDiseaseRect.width()* projectSetInfo.dRadioX;		    //病害宽度
		newDisease.dLength = newDiseaseRect.height()* projectSetInfo.dRadioY;		//病害长度

		newDisease.nPixelWid = newDiseaseRect.width();								//病害像素宽度
		newDisease.nPixelLen = newDiseaseRect.height();								//病害像素长度
		newDisease.dArea = newDisease.dReaWidth * newDisease.dRealLen;				//病害面积

		//计算2d的坐标信息
		vector<hn2dRectI> disease2dPointVector = this->generateLargeFrameHn2dRectVector(newDiseaseRect);
		newDisease.vec2dRect = disease2dPointVector;

		//清空3d坐标信息
		newDisease.vec3dRect.clear();

		//如果打开了3d工程，才进行映射
		if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject() && m_isOPenDepthCaculate/* && false == newDisease.vec3dRect.empty()*/)
		{
			//计算3d的坐标信息
			vector<hn3dRectI> disease3dPointVector = this->generateLargeFrameHn3dRectVector(newDiseaseRect);

			//如果映射后的3d坐标数组无效，则提示用户
			if (disease3dPointVector.size() == 0)
			{
				QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
					QString::fromLocal8Bit("确定"));
			}
			else
			{
				newDisease.vec3dRect = disease3dPointVector;

				if (false == newDisease.vec3dRect.empty())
				{
					//人工模式合并病害深度计算
					if (false == this->caculateBigFrameDiseaseDepth(newDisease))
					{
						m_seclectedDiseases.clear();
						return;
					}
				}

			}
		}

		//中心里程
		newDisease.dMileage = this->calculateBigFrameCenterMile(newDiseaseRect);
		//开始里程
		newDisease.dDmiStart = this->calculateBigFrameBeginMile(newDiseaseRect);
		//结束里程
		newDisease.dDmiEnd = this->calculateBigFrameEndMile(newDiseaseRect);

		//计算病害的计算面积 
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(newDisease);

		//重新设置ID
		newDisease.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->getDiseaseTable()->getMaxID(newDisease.strDiseaseTableName);

		//新病害写入数据库



		if (!validateLineCameraDiseaseGeometry(newDisease, this))
		{
			return;
		}

		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0); 

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1); 

		if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(newDisease))
		{
			QMessageBox::warning(this, QStringLiteral("\u8b66\u544a"), QStringLiteral("\u5408\u5e76\u75c5\u5bb3\u5199\u5165\u5931\u8d25\uff0c\u539f\u75c5\u5bb3\u672a\u5220\u9664\u3002"), QStringLiteral("\u786e\u5b9a"));
			m_seclectedDiseases.clear();
			return;
		}
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);
		 
		this->update();
		 
		m_seclectedDiseases.clear();
	}
}

QVector<QRect> hn2dPixWidget::sdkDiseaseBigImageRects(const hnRoadDiseaseInfo& disease)
{
	QVector<QRect> rects;
	if (disease.vec2dRect.empty())
	{
		return rects;
	}

	for (const hn2dRectI& rect2d : qAsConst(disease.vec2dRect))
	{
		rects.push_back(hn2dRectToImageQtRect(rect2d));
	}
	return rects;
}
QPainterPath hn2dPixWidget::sdkDiseaseScenePath(const hnRoadDiseaseInfo& disease)
{
	if (disease.vec2dRect.empty())
	{
		return QPainterPath();
	}

	QPainterPath cachedPath;
	if (cachedSdkLittleFrameScenePath(disease, cachedPath))
	{
		return cachedPath;
	}

	const bool isLittleFrame = disease.nDrawType == 1;
	auto imageNameFor2dPoint = [this, &disease](const hn2dPointWithMileI& point, QString& imageName)->bool
	{
		QVector<double> mileCandidates;
		auto appendMile = [&mileCandidates](double mile)
		{
			if (mile >= 0.0 && !mileCandidates.contains(mile))
			{
				mileCandidates.append(mile);
			}
		};

		appendMile(point.m_dmi);
		appendMile(disease.dDmi);
		appendMile(disease.dDmiStart);
		appendMile(disease.dMileage);
		appendMile(disease.dDmiEnd);

		for (double mile : qAsConst(mileCandidates))
		{
			if (resolve2dDiseaseImageNameByMile(mile, imageName))
			{
				return true;
			}
		}
		return false;
	};

	auto displayedPoint = [this](const hn2dPointWithMileI& point)->QPoint
	{
		int x = point.x;
		int y = point.y;
		if (m_isHMirrored)
		{
			x = m_pixWidth - 1 - point.x;
		}
		if (m_isVMirrored)
		{
			y = m_pixHeight - 1 - point.y;
		}
		return QPoint(qBound(0, x, qMax(0, m_pixWidth - 1)),
			qBound(0, y, qMax(0, m_pixHeight - 1)));
	};

	auto pointToScene = [this, &imageNameFor2dPoint, &displayedPoint](const hn2dPointWithMileI& point, QPointF& scenePoint)->bool
	{
		QString imageName;
		if (!imageNameFor2dPoint(point, imageName))
		{
			return false;
		}
		const QPoint imagePoint = displayedPoint(point);
		scenePoint = sdkPixPointToScenePoint(pixImagePoint{ imageName, imagePoint });
		return true;
	};

	if (disease.nDrawType == 3)
	{
		QPainterPath linePath;
		bool hasStart = false;
		for (const hn2dRectI& rect : qAsConst(disease.vec2dRect))
		{
			QPointF scenePoint;
			if (!pointToScene(rect.p0, scenePoint))
			{
				continue;
			}
			if (!hasStart)
			{
				linePath.moveTo(scenePoint);
				hasStart = true;
			}
			else
			{
				linePath.lineTo(scenePoint);
			}
		}
		return linePath;
	}

	QPainterPath path;
	QRectF bigFrameSceneBounds;
	bool hasBigFrameSceneBounds = false;
	QVector<LittleFrameRenderCell> littleFrameCells;
	littleFrameCells.reserve(disease.vec2dRect.size());
	for (const hn2dRectI& rect : qAsConst(disease.vec2dRect))
	{
		if (isLittleFrame)
		{
			QString imageName;
			if (!imageNameFor2dPoint(rect.p0, imageName))
			{
				continue;
			}
			const QPoint sourceP0 = displayedPoint(rect.p0);
			const QPoint sourceP1 = displayedPoint(rect.p1);
			const QPoint sourceP2 = displayedPoint(rect.p2);
			const QPoint sourceP3 = displayedPoint(rect.p3);
			QRect sourceRect(sourceP0, sourceP0);
			sourceRect = sourceRect.united(QRect(sourceP1, sourceP1));
			sourceRect = sourceRect.united(QRect(sourceP2, sourceP2));
			sourceRect = sourceRect.united(QRect(sourceP3, sourceP3)).normalized();

			// Map one stable anchor per cell. Scene item coordinates use image pixels,
			// so deriving the remaining corner from local width/height prevents a y=0
			// boundary corner from being resolved onto the adjacent image.
			const QPointF sceneAnchor = sdkPixPointToScenePoint(
				pixImagePoint{ imageName, sourceRect.topLeft() });
			const QRectF sceneRect(sceneAnchor,
				sceneAnchor + QPointF(sourceRect.right() - sourceRect.left(),
					sourceRect.bottom() - sourceRect.top()));

			LittleFrameRenderCell cell;
			cell.imageName = imageName;
			cell.sourceRect = sourceRect;
			cell.sceneRect = sceneRect;
			if (cell.isValid())
			{
				littleFrameCells.append(cell);
			}
		}
		else
		{
			QPointF p0, p1, p2, p3;
			if (!pointToScene(rect.p0, p0) || !pointToScene(rect.p1, p1) ||
				!pointToScene(rect.p2, p2) || !pointToScene(rect.p3, p3))
			{
				continue;
			}
			const qreal left = qMin(qMin(p0.x(), p1.x()), qMin(p2.x(), p3.x()));
			const qreal right = qMax(qMax(p0.x(), p1.x()), qMax(p2.x(), p3.x()));
			const qreal top = qMin(qMin(p0.y(), p1.y()), qMin(p2.y(), p3.y()));
			const qreal bottom = qMax(qMax(p0.y(), p1.y()), qMax(p2.y(), p3.y()));
			const QRectF segmentBounds(QPointF(left, top), QPointF(right, bottom));
			bigFrameSceneBounds = hasBigFrameSceneBounds
				? bigFrameSceneBounds.united(segmentBounds)
				: segmentBounds;
			hasBigFrameSceneBounds = true;
		}
	}

	if (!isLittleFrame)
	{
		QPainterPath outline;
		if (hasBigFrameSceneBounds && bigFrameSceneBounds.isValid() && !bigFrameSceneBounds.isNull())
		{
			outline.addRect(bigFrameSceneBounds);
		}
		return outline;
	}

	const LittleFrameRenderResult renderResult = LittleFrameRenderPathBuilder::build(littleFrameCells);
	if (!renderResult.path.isEmpty())
	{
		const SdkLittleFrameRenderMode cacheMode = renderResult.mode == LittleFrameRenderExactCells
			? SdkLittleFrameRenderSparseCells
			: SdkLittleFrameRenderDenseRect;
		cacheSdkLittleFrameScenePath(disease, renderResult.path, cacheMode);
		return renderResult.path;
	}

	const QPainterPath fallbackPath = hn2d3dPixBaseWidget::sdkDiseaseScenePath(disease);
	if (!fallbackPath.isEmpty())
	{
		cacheSdkLittleFrameScenePath(disease, fallbackPath, SdkLittleFrameRenderSparseCells);
	}
	return fallbackPath;
}
int hn2dPixWidget::sdkLittleFrameHitIndex(const hnRoadDiseaseInfo& disease, const pixImagePoint& point)
{
	if (disease.vec2dRect.empty() || point.pixName.isEmpty())
	{
		return -1;
	}

	QString hitImageName;
	if (!resolveSdkImageName(point.pixName, hitImageName))
	{
		hitImageName = point.pixName;
	}
	const QString hitFileName = QFileInfo(hitImageName).fileName();

	auto displayedPoint = [this](const hn2dPointWithMileI& src)->QPoint
	{
		int x = src.x;
		int y = src.y;
		if (m_isHMirrored)
		{
			x = m_pixWidth - 1 - src.x;
		}
		if (m_isVMirrored)
		{
			y = m_pixHeight - 1 - src.y;
		}
		return QPoint(qBound(0, x, qMax(0, m_pixWidth - 1)), qBound(0, y, qMax(0, m_pixHeight - 1)));
	};

	for (int i = 0; i < disease.vec2dRect.size(); ++i)
	{
		const hn2dRectI& rect = disease.vec2dRect[i];
		QString sourceImageName;
		if (!resolve2dDiseaseImageNameByMile(rect.p0.m_dmi, sourceImageName))
		{
			continue;
		}

		QString rectImageName;
		if (!resolveSdkImageName(sourceImageName, rectImageName))
		{
			rectImageName = sourceImageName;
		}
		if (QFileInfo(rectImageName).fileName() != hitFileName)
		{
			continue;
		}

		const QPoint p0 = displayedPoint(rect.p0);
		const QPoint p1 = displayedPoint(rect.p1);
		const QPoint p2 = displayedPoint(rect.p2);
		const QPoint p3 = displayedPoint(rect.p3);
		const int left = qMin(qMin(p0.x(), p1.x()), qMin(p2.x(), p3.x()));
		const int right = qMax(qMax(p0.x(), p1.x()), qMax(p2.x(), p3.x()));
		const int top = qMin(qMin(p0.y(), p1.y()), qMin(p2.y(), p3.y()));
		const int bottom = qMax(qMax(p0.y(), p1.y()), qMax(p2.y(), p3.y()));
		const QRect singleRect(QPoint(left, top), QPoint(right, bottom));
		if (singleRect.normalized().adjusted(-2, -2, 2, 2).contains(point.pixPoint))
		{
			return i;
		}
	}
	return -1;
}

bool hn2dPixWidget::sdkLittleFrameCellSceneRect(const hnRoadDiseaseInfo& disease, int hitIndex, QRectF& sceneRect)
{
	if (hitIndex < 0 || hitIndex >= disease.vec2dRect.size())
	{
		return false;
	}

	auto imageNameFor2dPoint = [this, &disease](const hn2dPointWithMileI& point, QString& imageName)->bool
	{
		QVector<double> mileCandidates;
		auto appendMile = [&mileCandidates](double mile)
		{
			if (mile >= 0.0 && !mileCandidates.contains(mile))
			{
				mileCandidates.append(mile);
			}
		};

		appendMile(point.m_dmi);
		appendMile(disease.dDmi);
		appendMile(disease.dDmiStart);
		appendMile(disease.dMileage);
		appendMile(disease.dDmiEnd);

		for (double mile : qAsConst(mileCandidates))
		{
			if (resolve2dDiseaseImageNameByMile(mile, imageName))
			{
				return true;
			}
		}
		return false;
	};

	auto displayedPoint = [this](const hn2dPointWithMileI& point)->QPoint
	{
		int x = point.x;
		int y = point.y;
		if (m_isHMirrored)
		{
			x = m_pixWidth - 1 - point.x;
		}
		if (m_isVMirrored)
		{
			y = m_pixHeight - 1 - point.y;
		}
		return QPoint(qBound(0, x, qMax(0, m_pixWidth - 1)),
			qBound(0, y, qMax(0, m_pixHeight - 1)));
	};

	const hn2dRectI& rect = disease.vec2dRect[hitIndex];
	QString imageName;
	if (!imageNameFor2dPoint(rect.p0, imageName))
	{
		return false;
	}
	const QPoint sourceP0 = displayedPoint(rect.p0);
	const QPoint sourceP1 = displayedPoint(rect.p1);
	const QPoint sourceP2 = displayedPoint(rect.p2);
	const QPoint sourceP3 = displayedPoint(rect.p3);
	QRect sourceRect(sourceP0, sourceP0);
	sourceRect = sourceRect.united(QRect(sourceP1, sourceP1));
	sourceRect = sourceRect.united(QRect(sourceP2, sourceP2));
	sourceRect = sourceRect.united(QRect(sourceP3, sourceP3)).normalized();
	const QPointF sceneAnchor = sdkPixPointToScenePoint(
		pixImagePoint{ imageName, sourceRect.topLeft() });
	sceneRect = QRectF(sceneAnchor,
		sceneAnchor + QPointF(sourceRect.right() - sourceRect.left(),
			sourceRect.bottom() - sourceRect.top())).normalized();
	return sceneRect.isValid() && !sceneRect.isNull();
}
void hn2dPixWidget::updateSdkLittleFrameDiseaseAfterCellDelete(hnRoadDiseaseInfo& disease)
{
	updateLittleFrameDisease(disease);
}
void hn2dPixWidget::drawDatabaseLoadData(QImage & image)
{
	QElapsedTimer timer;

	timer.restart();
	if (!this->m_isAllowDrawPix)
		return;  

	auto project = hnDataManager::getDataManager()->getCurrentProject();

	QVector<hnRoadDiseaseInfo> diss;
	 
	timer.start();
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->getRoadDiseasesInRange(m_beginEncoderMile, m_endEncoderMile, diss);
	this->m_currentWidgetDiseases = diss.toStdVector();
	const qint64 getDiseases = timer.elapsed();
	timer.restart();
	if (this->m_frameMode == FrameMode::BIG_FRAME)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image, 0);
	}

	//将加载后的自动化模式病害画到界面上
	timer.start();
	if (this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		this->drawLittleFrameDisease(this->m_currentWidgetDiseases, image);
	}
	const qint64 drawMs = timer.elapsed();
	timer.restart();
	//qDebug() << "PERF drawLittleFrameDisease" << timer.elapsed() << "ms";
	//绘制设计模式面状病害
	if (FrameMode::DESIGN_FACETS == m_frameMode || FrameMode::DESIGN_LINE == m_frameMode)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image, 2);

		//绘制设计模式线状病害
		this->drawLineDiseases(m_currentWidgetDiseases, image);
	}

	timer.start();

	//绘制二三维开始里程矫正的线
	if (PROJECT_23D_TYPE == project->getProjectType())
	{
		if (false == m_seclectPoint.pixName.isEmpty())
		{
			QPoint startPoint = this->singleImagePointToBigImagePoint(QPoint(0, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QPoint endPoint = this->singleImagePointToBigImagePoint(QPoint(m_pixWidth, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QLine line(startPoint, endPoint);
			this->drawLineOnImage(line, 20, Qt::yellow, image);
		}
	
	
	}
	//记录临时内容画板
	m_tmpContectImage = image;



	const qint64 drawLineAndDrawBigImage = timer.elapsed();
	timer.restart();
	#ifdef _DEBUG
	qDebug().noquote() << "[HN_PERF][drawDatabaseLoadData]"<< "getDiseases=" << getDiseases
		<<"[HN_PERF][drawDatabaseLoadData]" << "drawMs=" << drawMs
		<< "[HN_PERF][drawDatabaseLoadData]" << "drawLineAndDrawBigImage=" << drawLineAndDrawBigImage
		;
	#endif
		 
}

void hn2dPixWidget::drawTmpData(QImage & image)
{ 
	if (!this->m_isAllowDrawPix)
	{
		//改变画板为临时内容画板
		image = this->m_tmpContectImage;
	}

	//画临时自动化模式矩形病害
	if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (this->littleDrawRectType)
		{
			QRect bigRect = this->drawTmpLittleBigFrameDisease(image);

		 
			QStringList visiblePixNames;
			QVector<pixImagePoint> visibleImagePoints;
			for (auto it = m_currentWidgetPixNames.constBegin();it != m_currentWidgetPixNames.constEnd();++it)
			{
				visiblePixNames.append(it.value());
			}

			if (visiblePixNames!=m_cachedVisibleLittleFramePixNames)
			{
				m_cachedVisibleLittleFramePixNames = visiblePixNames;
				visibleImagePoints.reserve(visiblePixNames.size());

				for (const QString& pixName :qAsConst(visiblePixNames))
				{
					pixImagePoint  point;
					point.pixName = pixName;
					point.pixPoint = QPoint(0,0);
					visibleImagePoints.append(point);
				}
				m_cachedVisibleLittleFrameRects = this->createLittleFrameRects(visibleImagePoints);
			}
			this->m_currentLittleFrameRects = m_cachedVisibleLittleFrameRects;


			hn2d3dCoordinates tool;
			QVector<QRect> currentRects = tool.crossRectOver(bigRect, this->m_currentLittleFrameRects);
			
			this->m_tmpLittleFrameDiseaseRects.clear();
			this->appendCommittedLittleRectDrawSelection(this->m_tmpLittleFrameDiseaseRects);
			if (this->m_tmpLittleFrameDiseaseRects.isEmpty())
			{
				this->m_tmpLittleFrameDiseaseRects = currentRects;
			}
			else
			{
				for (const QRect& rect : qAsConst(currentRects))
				{
					if (!this->m_tmpLittleFrameDiseaseRects.contains(rect))
					{
						this->m_tmpLittleFrameDiseaseRects.append(rect);
					}
				}
			}


			



			const QColor rectColor = Qt::red;
			this->drawRectsOnImage(
				image,
				this->m_tmpLittleFrameDiseaseRects,
				diseaseImagePixels(image, m_diseaseDrawStyle.tempRectWidth),
				rectColor,
				Qt::SolidLine);

		}
		else
		{
			if (this->addLineDiseType)
			{
				if (m_littleSingleImagePoints.size() > 1)
				{
					QVector<pixImagePoint> tmpLittleSingleImagePoints = m_littleSingleImagePoints;

					for (int i = 1; i < m_littleSingleImagePoints.size(); ++i)
					{
						QPoint startPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i - 1].pixPoint, m_littleSingleImagePoints[i - 1].pixName);
						QPoint endPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i].pixPoint, m_littleSingleImagePoints[i].pixName);
						QLine line(startPoint, endPoint);
						QPainter painter(&image);
						QPen pen;
						pen.setColor(Qt::yellow);
						pen.setWidth(m_diseaseDrawStyle.tempRectWidth);
						pen.setStyle(Qt::DashLine);
						painter.setPen(pen);
						painter.drawLine(line);

						//获取图片名字
						QString startName = m_littleSingleImagePoints[i - 1].pixName;
						QString endName = m_littleSingleImagePoints[i].pixName;

						//根据图片名字获取帧序号
						int startIdx = m_pixNameMap.key(startName, -1);
						int endIdx = m_pixNameMap.key(endName, -1);

						//如果结束的帧号小，就交换一下
						if (qMin(startIdx, endIdx) == endIdx)
						{
							qSwap(startIdx, endIdx);
						}

						//左键连续点击添加病害模式下，若相邻点击点之间存在跨多个单张图像时，需额外补充点以生成完整的小方框
						if ((endIdx - startIdx) > 1)
						{
							for (int i = startIdx + 1; i < endIdx; i++)
							{
								QString pixName = m_pixNameMap.value(i, "");
								if (!pixName.isEmpty())
								{
									pixImagePoint tmpInsertPoint;
									tmpInsertPoint.pixName = pixName;
									tmpInsertPoint.pixPoint = QPoint(100, 100);
									tmpLittleSingleImagePoints.append(tmpInsertPoint);
								}
							}
						}

						//生成多段折线病害自动化模式
						this->m_currentLittleFrameRects = this->createLittleFrameRects(tmpLittleSingleImagePoints);

						//判断鼠标移动轨迹折线与当前自动化模式矩形数组 相交的矩形数组
						hn2d3dCoordinates tool;
						QVector<QRect> tmpDiseaseRects = tool.crossLineOver(line, this->m_currentLittleFrameRects);

						//画出自动化模式
						const QColor rectColor = Qt::red;
					  

						this->drawRectsOnImage(image, QVector<QRect>::fromList(tmpDiseaseRects.toList()), diseaseImagePixels(image,m_diseaseDrawStyle.tempRectWidth), rectColor, Qt::SolidLine);

					}
				}


			}
			else  //画自动跟踪鼠标移动轨迹模式下病害自动化模式
			{
				//创造自动化模式鼠标移动轨迹经过的自动化模式数组
				m_currentLittleFrameRects = this->createLittleFrameRects(m_littleSingleImagePoints);

				//计算当前视图所有自动化模式与鼠标移动轨迹相交的矩形框
				hn2d3dCoordinates tool;
				this->m_tmpLittleFrameDiseaseRects = tool.crossOver(m_litteBigImagePoints, this->m_currentLittleFrameRects);

				//画出自动化模式
				const QColor rectColor = Qt::red; 
				this->drawRectsOnImage(image, this->m_tmpLittleFrameDiseaseRects, diseaseImagePixels(image,   m_diseaseDrawStyle.tempRectWidth), rectColor, Qt::SolidLine);

			}

		}
	}

	//画临时人工模式矩形病害
	if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::BIG_FRAME)
	{
		this->drawTmpBigFrameDisease(image);
	}

	//画临时设计模式面状病害
	if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::DESIGN_FACETS)
	{
		this->drawTmpBigFrameDisease(image);
	}
	//绘制临时设计模式线状病害
	if (this->m_isDrawingDisease &&  FrameMode::DESIGN_LINE == this->m_frameMode)
	{
		this->drawTmpLineDiseases(image);
	}
}


QRect hn2dPixWidget::hn2dRectToImageQtRect(const hn2dRectI & hnRect)
{
	QRect qrect;

	qrect.setTopLeft(this->hn2dPointToImageQtPoint(hnRect.p0));
	qrect.setTopRight(this->hn2dPointToImageQtPoint(hnRect.p1));
	qrect.setBottomRight(this->hn2dPointToImageQtPoint(hnRect.p2));
	qrect.setBottomLeft(this->hn2dPointToImageQtPoint(hnRect.p3));

	return qrect;
}

QPoint hn2dPixWidget::hn2dPointToImageQtPoint(const hn2dPointWithMileI & hn2dPoint)
{
	QPoint qpoint;
	//auto iter = this->m_milePixNameMap.find(hn2dPoint.m_dmi); 
	auto iter = getPreviousStakeIterator(m_milePixNameMap, hn2dPoint.m_dmi);
	if (iter == this->m_milePixNameMap.end())
	{
		return qpoint;
	}
	QString pixName = iter.value();

	//镜像的x，镜像的y
	int mirroredX = hn2dPoint.x;
	int mirroredY = hn2dPoint.y;
	 
	if (m_isHMirrored)
	{
		mirroredX = m_pixWidth - hn2dPoint.x;
	}
	if (m_isVMirrored)
	{
		mirroredY = m_pixHeight - hn2dPoint.y;
	}

	// 上面考虑翻转，这里再将点在小图坐标转为大图坐标
	qpoint = this->singleImagePointToBigImagePoint(QPoint(mirroredX, mirroredY), pixName);

	return qpoint;
}

void hn2dPixWidget::editDisease(hnRoadDiseaseInfo & disease, const QPoint & mousePoint)
{
	const hnRoadDiseaseInfo originalDisease = disease;
	hnMile mile = this->getHnMileFromPoint(mousePoint);

	//弹出添加病害窗口 让用户选择病害类型
	QStringList diseaseTypeList;
	QList<QPair<QString, QString>> diseaseNameAndKey;
	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	addDiseaseDialog dialog(diseaseNameAndKey, true);
	dialog.setDiseaseInfo(disease.dLength, disease.dWidth, disease.dArea, disease.dDepth);
	dialog.setWindowTitle(QString::fromLocal8Bit("编辑病害"));
	dialog.setDiseaseAttributeEnabled(false);

	//获取病害类型
	QString diseaseTypeName;
	QString diseaseMark;
	if (dialog.exec() == QDialog::Accepted)
	{
		diseaseTypeName = dialog.getDiseaseTypeName();
		diseaseMark = dialog.getDiseaseMarkInfo();
	}
	else
	{
		m_isDrawingDisease = false;
		return;
	}

 

	//给病害赋值病害类型
	strcpy(disease.strDisName, diseaseTypeName.toLocal8Bit().data());

	//计算病害等级
	if (diseaseTypeName.contains(QString::fromLocal8Bit("轻")))
	{
		disease.nLevel = 1;
	}
	else if (diseaseTypeName.contains(QString::fromLocal8Bit("中")))
	{
		disease.nLevel = 2;
	}
	else if (diseaseTypeName.contains(QString::fromLocal8Bit("重")))
	{
		disease.nLevel = 3;
	}
	else
	{
		disease.nLevel = 0;
	}

	//获取病害表名
	QString diseaseTableName = hnApp::hnDataManager::getDataManager()->getTableName(diseaseTypeName);
	//病害赋值表名
	strcpy(disease.strDiseaseTableName, diseaseTableName.toLocal8Bit().data());
	// 获取最大ID
	const int id = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getDiseaseTable()->getMaxID(std::string(disease.strDiseaseTableName));
	disease.nID = id;

	//计算病害面积信息
	hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	//变形类病害深度计算
	if (false == disease.vec3dRect.empty()
		&& true == m_isOPenDepthCaculate
		&& nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算人工模式病害深度信息
		if (0 == disease.nDrawType)
		{
			if (!this->caculateBigFrameDiseaseDepth(disease))
			{
				return;
			}
		}
		else if (1 == disease.nDrawType)
		{
			//计算自动化模式病害深度信息
			bool isDrawDisease = this->caculateLittleFrameDiseaseDepth(disease,
				*this,
				&rectAlgorithm::mergeRects,
				*this,
				&drawDiseases::generateLargeFrameHn3dRectVector,
				m_tmpLittleFrameDiseaseRects);
			if (false == isDrawDisease)
			{
				return;
			}
		}
	}
	//写入数据库
	auto mark8Bit = diseaseMark.toLocal8Bit();
	auto markStd = mark8Bit.toStdString();
	strcpy(disease.strRemark, markStd.c_str());

	if (!validateLineCameraDiseaseGeometry(disease, this))
	{
		return;
	}
	hnDiseaseService* service =
		hnApp::hnDataManager::getDataManager()->getDiseaseService();
	if (!service->addDisease(disease))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE7\x97\x85\xE5\xAE\xB3\xE7\xB1\xBB\xE5\x9E\x8B\xE4\xBF\xAE\xE6\x94\xB9\xE5\x86\x99\xE5\x85\xA5\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x8C\xE5\x8E\x9F\xE7\x97\x85\xE5\xAE\xB3\xE5\xB7\xB2\xE4\xBF\x9D\xE7\x95\x99\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
		return;
	}
	hnRoadDiseaseInfo oldDisease = originalDisease;
	if (!service->deleteOneDisease(oldDisease))
	{
		QMessageBox::warning(this, QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA"),
			QString::fromUtf8("\xE6\x96\xB0\xE7\x97\x85\xE5\xAE\xB3\xE5\xB7\xB2\xE4\xBF\x9D\xE5\xAD\x98\xEF\xBC\x8C\xE4\xBD\x86\xE6\x97\xA7\xE7\x97\x85\xE5\xAE\xB3\xE5\x88\xA0\xE9\x99\xA4\xE5\xA4\xB1\xE8\xB4\xA5\xEF\xBC\x8C\xE8\xAF\xB7\xE5\x88\xB7\xE6\x96\xB0\xE5\x88\x97\xE8\xA1\xA8\xE5\x90\x8E\xE6\xA3\x80\xE6\x9F\xA5\xE3\x80\x82"), QString::fromUtf8("\xE7\xA1\xAE\xE5\xAE\x9A"));
	}

	m_isAllowDrawPix = true;
	this->update();
}

bool hn2dPixWidget::updateSdkAreaDiseaseGeometry(hnRoadDiseaseInfo& disease, bool saveToDatabase)
{
	QVector<SdkSingleImageRect> sdkRects;
	if (!currentSdkBigFrameSingleRects(sdkRects) || sdkRects.isEmpty())
	{
		return false;
	}
	const QRect legacyInput = sdkRects.first().singleRect.normalized();
	disease.vec2dRect = generateLargeFrameHn2dRectVector(legacyInput);
	if (disease.vec2dRect.empty())
	{
		return false;
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		const vector<hn3dRectI> mapped = generateLargeFrameHn3dRectVector(legacyInput);
		if (!mapped.empty())
		{
			disease.vec3dRect = mapped;
		}
	}
	const hnProjectSetInfo setting =
		hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	const double beginMile = sdkPixPointToEncoderMile(m_diseaseStartPoint);
	const double endMile = sdkPixPointToEncoderMile(m_diseaseEndPoint);
	disease.nPixelWid = qAbs(m_diseaseEndPoint.pixPoint.x() - m_diseaseStartPoint.pixPoint.x());
	disease.dWidth = std::round(disease.nPixelWid * setting.dRadioX * 100.0) / 100.0;
	disease.dLength = std::round(qAbs(endMile - beginMile) * 100.0) / 100.0;
	disease.nPixelLen = setting.dRadioY > 0.0
		? qRound(disease.dLength / setting.dRadioY) : 0;
	disease.dDmiStart = qMin(beginMile, endMile);
	disease.dDmiEnd = qMax(beginMile, endMile);
	disease.dMileage = (disease.dDmiStart + disease.dDmiEnd) * 0.5;
	disease.dDmi = disease.vec2dRect.front().p0.m_dmi;
	hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	if (!validateDiseaseGeometryWithinValidArea(disease)
		|| !validateLineCameraDiseaseGeometry(disease, this))
	{
		return false;
	}
	return !saveToDatabase || hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);
}

bool hn2dPixWidget::moveSdkLittleFrameDisease(hnRoadDiseaseInfo& disease, const QPoint& bigImageOffset)
{
	if (disease.nDrawType != 1)
	{
		return false;
	}
	if (bigImageOffset.isNull())
	{
		return true;
	}
	QVector<QRect> movedRects = sdkDiseaseBigImageRects(disease);
	if (movedRects.isEmpty())
	{
		return false;
	}
	for (QRect& rect : movedRects)
	{
		rect.translate(bigImageOffset);
	}

	m_committedLittleFrameDiseaseRects.clear();
	rebuildCurrentLittleFrameSingleSelections(movedRects);
	if (m_currentLittleFrameSingleSelections.size() != movedRects.size())
	{
		return false;
	}
	const vector<hn2dRectI> moved2d = generateLittleFrameHn2dRectVector(movedRects);
	if (moved2d.size() != static_cast<size_t>(movedRects.size()))
	{
		return false;
	}
	disease.vec2dRect = moved2d;

	auto project = hnDataManager::getDataManager()->getCurrentProject();
	if (project && project->get3DProject())
	{
		const vector<hn3dRectI> moved3d = generateLittleFrameHn3dRectVector(movedRects);
		if (moved3d.empty())
		{
			return false;
		}
		disease.vec3dRect = moved3d;
	}

	disease.dMileage = caculateLittleFrameMiddleMile(movedRects);
	disease.dDmiStart = calculateLittleFrameBeginMile(movedRects);
	disease.dDmiEnd = calculateLittleFrameEndMile(movedRects);
	disease.dDmi = disease.vec2dRect.front().p0.m_dmi;
	CalculateDiseaseSize(movedRects, disease);
	hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	if (!validateDiseaseGeometryWithinValidArea(disease)
		|| !validateLineCameraDiseaseGeometry(disease, this))
	{
		return false;
	}
	return hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);
}
void hn2dPixWidget::updateLittleFrameDisease(hnRoadDiseaseInfo & disease)
{
	PROJECT_TYPE projectType=  hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	// 如果自动化模式数量为0 ，则直接删除整个病害
	if (disease.vec2dRect.size() <= 0 || (disease.vec3dRect.size() <= 0&& projectType!= PROJECT_TYPE::PROJECT_2D_TYPE))
	{ 
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease); 
		this->update();
		return;
	}

	//计算病害面积信息
	hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	//变形类病害深度计算
	if (false == disease.vec3dRect.empty()
		&& true == m_isOPenDepthCaculate
		&& nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算人工模式病害深度信息
		if (0 == disease.nDrawType)
		{
			if (!this->caculateBigFrameDiseaseDepth(disease))
			{
				return;
			}
		}
		else if (1 == disease.nDrawType)
		{
			//计算自动化模式病害深度信息
			bool isDrawDisease = this->caculateLittleFrameDiseaseDepth(disease,
				*this,
				&rectAlgorithm::mergeRects,
				*this,
				&drawDiseases::generateLargeFrameHn3dRectVector,
				m_tmpLittleFrameDiseaseRects);
			if (false == isDrawDisease)
			{
				return;
			}
		}

	}
	auto setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	// todo 经过测试主要耗时是读写数据库耗时，除非将病害变为临时病害，在鼠标释放事件写入数据库，才会流畅一些
	//写入数据库
	if (!validateLineCameraDiseaseGeometry(disease, this))
	{
		return;
	}
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);

	 
	m_isAllowDrawPix = true;
	this->update();

}

double hn2dPixWidget::calculateEncoderMile(const QPoint & bigImagePoint)
{
	double encoderMile;

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	QString pixName;
	hnMile bottomHnMile = getHnMileFromPoint(bigImagePoint);
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	encoderMile = (projectSetInfo.picPixelY - singleImagePoint.y()) * projectSetInfo.dRadioY + bottomHnMile.dEnclMile;

	return encoderMile;
}
double hn2dPixWidget::caculateTrueMile(const QPoint & bigImagePoint)
{
	const double encoderMile = this->calculateEncoderMile(bigImagePoint);

	const double trueMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(encoderMile);

	return trueMile;
}

QString hn2dPixWidget::generateStatusInfo(const QPoint & eventPos)
{
	QString statusInfo;
	QPoint bigImagePoint = this->screenPointToBigImagePoint(eventPos);
	QString pixName;
	QPoint singleImagePoint = this->screenToSingleImagePoint(eventPos, pixName);
    const int imageExtentX = qMax(0, m_pixWidth);
    const int imageExtentY = qMax(0, m_pixHeight);
    int originalSingleX = qBound(0, singleImagePoint.x(), imageExtentX);
    int originalSingleY = qBound(0, singleImagePoint.y(), imageExtentY);
    if (m_isHMirrored)
    {
        originalSingleX = imageExtentX - originalSingleX;
    }
    if (m_isVMirrored)
    {
        originalSingleY = imageExtentY - originalSingleY;
    }
	const QPoint statusSingleImagePoint(originalSingleX, originalSingleY);
	QFileInfo fileInfo(pixName);
	QString pixNameResult = fileInfo.fileName();

    hnMile currentPointMile = this->getHnMileFromPoint(bigImagePoint);
    const double trueMile = caculateTrueMile(bigImagePoint);
    const double encoderMile = calculateEncoderMile(bigImagePoint);
    auto statusProject = hnDataManager::getDataManager()->getCurrentProject();
    const double roadWidth = statusProject ? statusProject->effectiveRoadWidth() : qMax(0.0, m_pixWidth * m_widthScale);
    const double rawXMile = statusProject && statusProject->isLineCameraProject()
        ? statusProject->getLineCameraInfo().roadXFromPixel(statusSingleImagePoint.x())
        : singleImagePoint.x() * m_widthScale;
    const double xMile = roadWidth > 0.0 ? qBound(0.0, rawXMile, roadWidth) : qMax(0.0, rawXMile);
    const QString mileText = QString::fromLocal8Bit("(X:%1,Y:%2)").arg(xMile, 0, 'f', 3).arg(encoderMile, 0, 'f', 3);
    QString streetPixName;
    if (hnDataManager::getDataManager()->getCurrentProject())
    {
        auto currentProject = hnDataManager::getDataManager()->getCurrentProject();
        const double streetTrueMile = currentProject->enclToTrueMile(currentBottomEncoderMile());
        streetPixName = currentStreetPictureNameForStatus(streetTrueMile);
    }
    statusInfo = QString::fromLocal8Bit("桩号：%1\t里程:%2\t路面标准：%3\t"
        "路面材质：%4\t路面等级：%5\t病害模式：%6\t拼接图片坐标：%7\t单张图片坐标：%8\t路面图片：%9\t景观图片：%10\t")
        .arg(trueMile, 0, 'f', 3)
        .arg(mileText)
        .arg(HnProjectEnums::roadTypeEnumToQString(currentPointMile.roadStandard))
        .arg(currentPointMile.roadType == 0 ? QString::fromLocal8Bit("沥青") :
        (currentPointMile.roadType == 1 ? QString::fromLocal8Bit("水泥") : QString::fromLocal8Bit("砂石")))
        .arg(currentPointMile.roadGradStr)
        .arg(currentPointMile.drawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式"))
        .arg(QString::number(bigImagePoint.x()) + "," + QString::number(bigImagePoint.y()))
        .arg(QString::number(statusSingleImagePoint.x()) + "," + QString::number(statusSingleImagePoint.y()))
        .arg(pixNameResult)
        .arg(streetPixName)
        ;

	return statusInfo;
}


vector<hn2dRectI> hn2dPixWidget::generateLargeFrameHn2dRectVector(const QRect & rect)
{
	std::vector<hn2dRectI> vec;

	if (hasSdkImageView())
	{
		QVector<SdkSingleImageRect> sdkRects;
		if (currentSdkBigFrameSingleRects(sdkRects))
		{
			auto toHnPoint = [this](const QString& pixName, const QPoint& point, hn2dPointWithMileI& out)->bool
			{
				QString projectImageName;
				if (!resolveSdkImageName(pixName, projectImageName))
				{
					projectImageName = pixName;
				}

				auto mileIter = m_pixNameHnMileMap.constFind(projectImageName);
				if (mileIter == m_pixNameHnMileMap.constEnd())
				{
					const QString fileName = QFileInfo(projectImageName).fileName();
					for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
					{
						if (QFileInfo(it.key()).fileName() == fileName)
						{
							mileIter = it;
							break;
						}
					}
				}
				if (mileIter == m_pixNameHnMileMap.constEnd())
				{
					return false;
				}

				int x = qBound(0, point.x(), qMax(0, m_pixWidth - 1));
				int y = qBound(0, point.y(), qMax(0, m_pixHeight - 1));
				if (m_isHMirrored)
				{
					x = m_pixWidth - 1 - x;
				}
				if (m_isVMirrored)
				{
					y = m_pixHeight - 1 - y;
				}
				out.x = x;
				out.y = y;
				out.m_dmi = mileIter.value().dEnclMile;
				return true;
			};

			for (const SdkSingleImageRect& sdkRect : qAsConst(sdkRects))
			{
				hn2dRectI hnRect;
				const QRect singleRect = sdkRect.singleRect.normalized();
				if (toHnPoint(sdkRect.pixName, singleRect.topLeft(), hnRect.p0) &&
					toHnPoint(sdkRect.pixName, singleRect.topRight(), hnRect.p1) &&
					toHnPoint(sdkRect.pixName, singleRect.bottomRight(), hnRect.p2) &&
					toHnPoint(sdkRect.pixName, singleRect.bottomLeft(), hnRect.p3))
				{
					vec.push_back(hnRect);
				}
			}
			return vec;
		}
	}

	hn2dRectI hnRect;
	hnRect.p0 = getHnPoint2dWithMileI(rect.topLeft());
	hnRect.p1 = getHnPoint2dWithMileI(rect.topRight());
	hnRect.p2 = getHnPoint2dWithMileI(rect.bottomRight());
	hnRect.p3 = getHnPoint2dWithMileI(rect.bottomLeft());

	vec.push_back(hnRect);
	return vec;
}
vector<hn2dRectI> hn2dPixWidget::createLineDisease2dCoordVec(QVector<pixImagePoint> lineDiseasePoints)
{
	vector<hn2dRectI> result;
	result.reserve(lineDiseasePoints.size());
	for (const pixImagePoint& lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QString projectImageName;
		if (!resolveSdkImageName(lineDiseasePoint.pixName, projectImageName))
		{
			projectImageName = lineDiseasePoint.pixName;
		}

		auto mileIter = m_pixNameHnMileMap.constFind(projectImageName);
		if (mileIter == m_pixNameHnMileMap.constEnd())
		{
			const QString fileName = QFileInfo(projectImageName).fileName();
			for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
			{
				if (QFileInfo(it.key()).fileName().compare(fileName, Qt::CaseInsensitive) == 0)
				{
					mileIter = it;
					break;
				}
			}
		}
		if (mileIter == m_pixNameHnMileMap.constEnd())
		{
			qWarning().noquote() << "[HN_DESIGN_LINE_STORE_SKIP] image=" << lineDiseasePoint.pixName;
			continue;
		}

		int x = qBound(0, lineDiseasePoint.pixPoint.x(), qMax(0, m_pixWidth - 1));
		int y = qBound(0, lineDiseasePoint.pixPoint.y(), qMax(0, m_pixHeight - 1));
		if (m_isHMirrored)
		{
			x = m_pixWidth - 1 - x;
		}
		if (m_isVMirrored)
		{
			y = m_pixHeight - 1 - y;
		}

		hn2dRectI hnRect = {};
		hnRect.p0.x = x;
		hnRect.p0.y = y;
		hnRect.p0.m_dmi = mileIter.value().dEnclMile;
		result.push_back(hnRect);
	}

	return result;
}

vector<hn3dRectI> hn2dPixWidget::createLineDisease3dCoordVec(QVector<pixImagePoint> lineDiseasePoints)
{
	vector<hn3dRectI> result;
	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QPoint bigImagePoint = this->singleImagePointToBigImagePoint(lineDiseasePoint.pixPoint, lineDiseasePoint.pixName);
		hn3dRectI hnRect = {};
		hnRect.p0 = getHnPoint3dWithMileI(bigImagePoint);
		if (hnRect.p0.bottomEncoderMile < 0)
		{
			continue;
		}
		result.push_back(hnRect);
	}

	return result;
}

hnCommon::hn2dPointWithMileI hn2dPixWidget::getHnPoint2dWithMileI(const QPoint & point)
{

	QString picName;

	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(point,&picName);

	int mirroredX = singleImagePoint.x();
	int mirroredY = singleImagePoint.y();
	if (m_isHMirrored)
	{
		mirroredX = m_pixWidth - mirroredX;
	}

	if (m_isVMirrored)
	{
		mirroredY = m_pixHeight - mirroredY;
	}

	hnCommon::hn2dPointWithMileI point2dWithMileI;
	point2dWithMileI.x = mirroredX;
	point2dWithMileI.y = mirroredY;
	point2dWithMileI.m_dmi = 0;


	auto iter = m_pixNameHnMileMap.constFind(picName);
	if (iter != m_pixNameHnMileMap.constEnd())
	{
		point2dWithMileI.m_dmi = iter.value().dEnclMile;
	}
	return point2dWithMileI;
}

//hnCommon::hn2dPointWithMileI hn2dPixWidget::getHnPoint2dWithMileI(const QPoint & point)
//{
//	QString picName;
//	QPoint singleImagePoint;
//	singleImagePoint = this->bigImagePointToSingleImagePoint(point, &picName);
//
//	// 注意：这个函数在
//	//镜像做处理
//	int mirroredX = singleImagePoint.x();
//	int mirroredY = singleImagePoint.y();
//	if (m_isHMirrored)
//	{
//		mirroredX = m_pixWidth - mirroredX;
//	}
//	if (m_isVMirrored)
//	{
//		mirroredY = m_pixHeight - mirroredY;
//	}
//
//	hnCommon::hn2dPointWithMileI point2dWithMileI;
//	point2dWithMileI.x = mirroredX;
//	point2dWithMileI.y = mirroredY;
//
//	for (auto iter = m_pixNameHnMileMap.begin(); iter != m_pixNameHnMileMap.end(); iter++)
//	{
//		if (iter.key().contains(picName))
//		{
//			point2dWithMileI.m_dmi = iter.value().dEnclMile;
//			break;
//		}
//	}
//	return point2dWithMileI;
//}

bool hn2dPixWidget::sdkHnMileFromPoint(const pixImagePoint& point, hnMile& mile) const
{
	if (point.pixName.isEmpty())
	{
		return false;
	}

	auto iter = m_pixNameHnMileMap.constFind(point.pixName);
	if (iter == m_pixNameHnMileMap.constEnd())
	{
		const QString fileName = QFileInfo(point.pixName).fileName();
		for (auto candidate = m_pixNameHnMileMap.constBegin(); candidate != m_pixNameHnMileMap.constEnd(); ++candidate)
		{
			if (QFileInfo(candidate.key()).fileName().compare(fileName, Qt::CaseInsensitive) == 0)
			{
				iter = candidate;
				break;
			}
		}
	}
	if (iter == m_pixNameHnMileMap.constEnd())
	{
		return false;
	}

	mile = iter.value();
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project)
	{
		return true;
	}
	const hnProjectSetInfo setting = project->getCurProSetInfo();
	const bool missingRoadContext = mile.roadWidth <= 0.0;
	if (missingRoadContext)
	{
		mile.roadWidth = project->effectiveRoadWidth();
		mile.roadStandard = HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard);
		mile.roadType = static_cast<hnCommon::ROAD_SURFACE_TYPE>(setting.nRSurfaceType);
	}
	if (static_cast<int>(mile.roadStandard) < static_cast<int>(HnProjectEnums::DegreeRoad2018) ||
		static_cast<int>(mile.roadStandard) > static_cast<int>(HnProjectEnums::RuralRoadlowLevel))
	{
		mile.roadStandard = HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard);
	}
	if (static_cast<int>(mile.roadType) < 0 || static_cast<int>(mile.roadType) > 2)
	{
		mile.roadType = static_cast<hnCommon::ROAD_SURFACE_TYPE>(setting.nRSurfaceType);
	}
	// 病害类型严格跟随当前绘制工具，避免首帧尚未初始化的 drawType 造成空选择框。
	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		mile.drawType = hnCommon::ROAD_WORK_SMALL_RECT;
	}
	else if (m_frameMode == FrameMode::DESIGN_FACETS || m_frameMode == FrameMode::DESIGN_LINE)
	{
		mile.drawType = hnCommon::DESIGN;
	}
	else
	{
		mile.drawType = hnCommon::ROAD_WORK_LARGE_RECT;
	}
	return true;
}
hnMile hn2dPixWidget::getHnMileFromPoint(const QPoint & allImagePoint)
{
	QString picName;
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(allImagePoint, &picName);
	Q_UNUSED(singleImagePoint);
	hnMile mile;

	auto iter = m_pixNameHnMileMap.constFind(picName);
	if (iter != m_pixNameHnMileMap.constEnd())
	{
		mile = iter.value();
	}
	/*for (auto iter = m_pixNameHnMileMap.begin(); iter != m_pixNameHnMileMap.end(); iter++)
	{
		if (iter.key().contains(picName))
		{
			mile = iter.value();
			break;
		}
	}*/
	return mile;
}


double hn2dPixWidget::calculateBigFrameCenterMile(const QRect & rect)
{
	double mile;

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile topHnMile = getHnMileFromPoint(rect.topLeft());
	QString pixName;
	QPoint topPoint = this->bigImagePointToSingleImagePoint(rect.topLeft(), &pixName);
	double topMile = (projectSetInfo.picPixelY - topPoint.y()) * projectSetInfo.dRadioY + topHnMile.dEnclMile;

	hnMile bottomHnMile = getHnMileFromPoint(rect.bottomLeft());
	QPoint bottomPoint = this->bigImagePointToSingleImagePoint(rect.bottomLeft(), &pixName);
	double bottomMile = (projectSetInfo.picPixelY - bottomPoint.y()) * projectSetInfo.dRadioY + bottomHnMile.dEnclMile;

	mile = (topMile + bottomMile) / 2;

	return mile;
}

double hn2dPixWidget::calculateBigFrameBeginMile(const QRect & rect)
{
	double beginMile;

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	QString pixName;
	hnMile bottomHnMile = getHnMileFromPoint(rect.bottomLeft());
	QPoint bottomPoint = this->bigImagePointToSingleImagePoint(rect.bottomLeft(), &pixName);
	double bottomMile = (projectSetInfo.picPixelY - bottomPoint.y()) * projectSetInfo.dRadioY + bottomHnMile.dEnclMile;
	beginMile = bottomMile;

	return beginMile;
}

double hn2dPixWidget::calculateBigFrameEndMile(const QRect & rect)
{
	double endMile;

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile topHnMile = getHnMileFromPoint(rect.topLeft());
	QString pixName;
	QPoint topPoint = this->bigImagePointToSingleImagePoint(rect.topLeft(), &pixName);
	double topMile = (projectSetInfo.picPixelY - topPoint.y()) * projectSetInfo.dRadioY + topHnMile.dEnclMile;

	endMile = topMile;

	return endMile;
}

void hn2dPixWidget::setCurrentHnMile()
{
	auto pixNameIter = this->m_pixNameMap.find((int)m_buttomFrameIdx);

	if (pixNameIter == this->m_pixNameMap.end())
	{
		return;
	}

	QString pixName = pixNameIter.value();

	auto hnMileIter = this->m_pixNameHnMileMap.find(pixName);

	if (hnMileIter == this->m_pixNameHnMileMap.end())
	{
		return;
	}

	hnMile resultHnMile = hnMileIter.value();

	hnApp::hnDataManager::getDataManager()->getCurrentProject()->setCurrentRoadMile(resultHnMile);

}

std::vector<hnMile> hn2dPixWidget::getCurrentWidgetHnMiles()
{
	std::vector<hnMile> resultHnMiles;

	//前后各多算五张 为了修正二三维中间hnMile的差值
	const int extraFrameNum = 20;
	int extraBeginFrame = m_buttomFrameIdx - extraFrameNum;
	int extraEndFrame = m_buttomFrameIdx + extraFrameNum;

	if (extraBeginFrame < 0)
	{
		extraBeginFrame = 0;
	}

	if (extraEndFrame > this->m_hnMileVector.size())
	{
		extraEndFrame = this->m_hnMileVector.size();
	}

	for (auto idx = extraBeginFrame; idx <= extraEndFrame; idx++)
	{
		if (idx < m_hnMileVector.size())
		{
			resultHnMiles.push_back(this->m_hnMileVector.at(idx));
		}
	}

	return resultHnMiles;
}

bool hn2dPixWidget::isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo & disease, const FrameMode & frameMode)
{
	//屏幕坐标转换成拼接后image的坐标
	QPoint imagePoint = this->screenPointToBigImagePoint(screenPoint);

	if (hnFrameMode::BIG_FRAME == frameMode || hnFrameMode::DESIGN_FACETS == frameMode)
	{
		if (disease.vec2dRect.empty())
		{
			return false;
		}
		hn2dRectI  hnRect = disease.vec2dRect.at(0);

		QRect diseaseRect = this->hn2dRectToImageQtRect(hnRect);

		if (diseaseRect.contains(imagePoint))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (frameMode == hnFrameMode::LITTLE_FRAME)
	{
		//获取病害的二维数组
		std::vector<hn2dRectI>  hn2dRects = disease.vec2dRect;

		//自动化模式病害的矩形数组
		QVector<QRect> diseaseRects;

		for (hn2dRectI rect2d : qAsConst(hn2dRects))
		{
			QRect rect = this->hn2dRectToImageQtRect(rect2d);
			diseaseRects.push_back(rect);
		}

		//遍历病害数组，看看是不是包含这个点
		for (auto rect : diseaseRects)
		{
			if (rect.contains(imagePoint))
			{
				return true;
			}
		}
	}
	//线状病害判断鼠标点是否在病害上
	else if (hnFrameMode::DESIGN_LINE == frameMode && 3 == disease.nDrawType)
	{
		return this->isNearbyLineDisease(imagePoint, disease);
	}
	return false;
}

double hn2dPixWidget::caculateEncoderMileByScreenPoint(const QPoint & screenPoint)
{
	double mile;

	QPoint bigImagePoint = this->screenPointToBigImagePoint(screenPoint);

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile hnmile = getHnMileFromPoint(bigImagePoint);
	QString pixName;
	QPoint singlePixPoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	mile = (projectSetInfo.picPixelY - singlePixPoint.y()) * projectSetInfo.dRadioY + hnmile.dEnclMile;

	return mile;
}

double hn2dPixWidget::caculateEncoderMileByBigImagePoint(const QPoint & bigImagePoint)
{
	double result;

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile hnmile = getHnMileFromPoint(bigImagePoint);
	QString pixName;
	QPoint singlePixPoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	result = (projectSetInfo.picPixelY - singlePixPoint.y()) * projectSetInfo.dRadioY + hnmile.dEnclMile;

	return result;
}

vector<hn3dRectI> hn2dPixWidget::generateLargeFrameHn3dRectVector(const QRect & rect)
{
	std::vector<hn3dRectI> vec;

	if (hasSdkImageView())
	{
		QVector<SdkSingleImageRect> sdkRects;
		if (currentSdkBigFrameSingleRects(sdkRects))
		{
			auto toHnPoint = [this](const QString& pixName, const QPoint& point, hn3dPointWithMileI& out)->bool
			{
				auto dataManager = hnDataManager::getDataManager();
				if (!dataManager || !dataManager->isOpenProject() ||
					!dataManager->getCurrentProject() || !dataManager->getCurrentProject()->get3DProject())
				{
					return false;
				}

				QString projectImageName;
				if (!resolveSdkImageName(pixName, projectImageName))
				{
					projectImageName = pixName;
				}

				auto mileIter = m_pixNameHnMileMap.constFind(projectImageName);
				if (mileIter == m_pixNameHnMileMap.constEnd())
				{
					const QString fileName = QFileInfo(projectImageName).fileName();
					for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
					{
						if (QFileInfo(it.key()).fileName() == fileName)
						{
							mileIter = it;
							break;
						}
					}
				}
				if (mileIter == m_pixNameHnMileMap.constEnd())
				{
					return false;
				}

				const hnProjectSetInfo projectSetInfo = dataManager->getCurrentProject()->getCurProSetInfo();
				const double point2dMile = mileIter.value().dEnclMile +
					(m_pixHeight - qBound(0, point.y(), qMax(0, m_pixHeight))) * projectSetInfo.dRadioY;
				const double target3dMile = qMax(0.0, point2dMile - dataManager->getCurrentProject()->get2d3dMileDiff());
				const double image3dHeightScale = dataManager->getCurrentProject()->get3DProject()->getImageHeightScale();
				if (image3dHeightScale <= 0.0)
				{
					return false;
				}

				const double bottom3dMile = qFloor(target3dMile / 8.0) * 8.0;
				const double offsetIn3dImage = target3dMile - bottom3dMile;
				hn2d3dCoordinates coordinates;
				int x = coordinates.single2dXToSingle3dX(qBound(0, point.x(), qMax(0, m_pixWidth - 1)));
				int y = qRound(dataManager->getCurrentProject()->get3DProject()->getImagePixelHeight() - offsetIn3dImage / image3dHeightScale);
				const int image3dWidth = dataManager->getCurrentProject()->get3DProject()->getImagePixelWidth();
				const int image3dHeight = dataManager->getCurrentProject()->get3DProject()->getImagePixelHeight();
				x = qBound(0, x, qMax(0, image3dWidth - 1));
				y = qBound(0, y, qMax(0, image3dHeight - 1));

				if (dataManager->getCurrentProject()->get3DProject()->getIsHMirrored())
				{
					x = image3dWidth - 1 - x;
				}
				if (dataManager->getCurrentProject()->get3DProject()->getIsVMirrored())
				{
					y = image3dHeight - 1 - y;
				}

				out.x = x;
				out.y = y;
				out.z = 0;
				out.bottomEncoderMile = bottom3dMile;
				return true;
			};

			for (const SdkSingleImageRect& sdkRect : qAsConst(sdkRects))
			{
				hn3dRectI hnRect;
				const QRect singleRect = sdkRect.singleRect.normalized();
				if (toHnPoint(sdkRect.pixName, singleRect.topLeft(), hnRect.p0) &&
					toHnPoint(sdkRect.pixName, singleRect.topRight(), hnRect.p1) &&
					toHnPoint(sdkRect.pixName, singleRect.bottomRight(), hnRect.p2) &&
					toHnPoint(sdkRect.pixName, singleRect.bottomLeft(), hnRect.p3))
				{
					vec.push_back(hnRect);
				}
			}
			return vec;
		}
	}

	hn3dRectI hnRect;
	hnRect.p0 = this->getHnPoint3dWithMileI(rect.topLeft());
	hnRect.p1 = this->getHnPoint3dWithMileI(rect.topRight());
	hnRect.p2 = this->getHnPoint3dWithMileI(rect.bottomRight());
	hnRect.p3 = this->getHnPoint3dWithMileI(rect.bottomLeft());

	if (hnRect.p0.bottomEncoderMile == -1 ||
		hnRect.p1.bottomEncoderMile == -1 ||
		hnRect.p2.bottomEncoderMile == -1 ||
		hnRect.p3.bottomEncoderMile == -1)
	{
		return vec;
	}

	vec.push_back(hnRect);
	return vec;
}
hnCommon::hn3dPointWithMileI hn2dPixWidget::getHnPoint3dWithMileI(const QPoint & point)
{
	//目标点
	hnCommon::hn3dPointWithMileI dstPoint;
	dstPoint.bottomEncoderMile = -1;
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return dstPoint;
	}
	//二三维有差值，要对其进行修正

	//获取二三维的编码器里程差值  
	double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();

	//转换成像素
	double yScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	int pixelDiff = encoderMileDiff / yScale;

	//获取新的点 
	QPoint new2dBigImagePoint = QPoint(point.x(), point.y() + pixelDiff);	//像素和里程方向是反的，所以，这里的y是加的

	//图片名称
	QString imageName;

	//单张2d图片内的坐标 我们这里认为用户已经把病害调到了对的位置，这里不对二维的点做处理
	QPoint single2dPoint = this->bigImagePointToSingleImagePoint(new2dBigImagePoint, &imageName);

	//如果该点无效，返回空的点
	if (imageName.isEmpty())
	{
		return dstPoint;
	}

	//四分之一3d图片内的坐标
	QPoint quarter3dPoint;
	//四分之一3d图片的宽度
	int quarter3dImageWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	//四分之一3d图片的高度
	int quarter3dImageHeight = 0.25 * hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	//赋值x 
	//这里对水平方向上二三维宽度的差异做了处理
	hn2d3dCoordinates coordinates;
	quarter3dPoint.setX(coordinates.single2dXToSingle3dX(single2dPoint.x()));
	dstPoint.x = quarter3dPoint.x();

	//赋值y
	quarter3dPoint.setY((single2dPoint.y() * quarter3dImageHeight) * 1.0 / this->m_pixHeight);
	hnMile singleImage2dHnMile;

	for (auto iter = this->m_pixNameHnMileMap.begin(); iter != this->m_pixNameHnMileMap.end(); iter++)
	{
		if (iter.key().contains(imageName))
		{
			singleImage2dHnMile = iter.value();
			break;
		}
	}
	double bottomEncoder2dMile = singleImage2dHnMile.dEnclMile;
	int encoderMileSingle3d = (int)((int)bottomEncoder2dMile) % 8;
	int quarter3dIdx = 3 - (encoderMileSingle3d / 2);
	dstPoint.y = quarter3dIdx * quarter3dImageHeight + quarter3dPoint.y();

	//赋值里程
	double heightScale3d = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	auto test = m_buttomFrameIdx;
	dstPoint.bottomEncoderMile = (int)(bottomEncoder2dMile / 8) * 8;

	//我们这里认为用户3维视图此时的翻转状态是一个正常的状态
	//根据翻转状态，来对三维视图的坐标进行一个调整

	//3d图片的宽度
	int image3dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	//3d图片的高度
	int image3dHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	//镜像处理
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored())
	{
		dstPoint.x = image3dWidth - dstPoint.x;
	}
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored())
	{
		dstPoint.y = image3dHeight - dstPoint.y;
	}

	return dstPoint;
}

void hn2dPixWidget::drawTmpBigFrameDisease(QImage &image)
{
	QPoint bigImageStart = singleImagePointToBigImagePoint(
		m_diseaseStartPoint.pixPoint,
		m_diseaseStartPoint.pixName);

	QPoint bigImageEnd = singleImagePointToBigImagePoint(
		m_diseaseEndPoint.pixPoint,
		m_diseaseEndPoint.pixName);

	QRect rect(bigImageStart, bigImageEnd);
	rect = rect.normalized();

	drawRectOnImageByStyle(
		image,
		rect,
		m_diseaseDrawStyle.tempRectWidth,
		m_diseaseDrawStyle.tempRectColor,
		Qt::SolidLine);
}

QRect hn2dPixWidget::drawTmpLittleBigFrameDisease(QImage &image)
{
	QPoint bigImageStart = singleImagePointToBigImagePoint(
		m_diseaseStartPoint.pixPoint,
		m_diseaseStartPoint.pixName);

	QPoint bigImageEnd = singleImagePointToBigImagePoint(
		m_diseaseEndPoint.pixPoint,
		m_diseaseEndPoint.pixName);

	QRect rect(bigImageStart, bigImageEnd);
	rect = rect.normalized();

	drawRectOnImageByStyle(
		image,
		rect,
		m_diseaseDrawStyle.tempRectWidth,
		m_diseaseDrawStyle.tempRectColor,
		Qt::SolidLine);

	return rect;
}

hnRoadDiseaseInfo hn2dPixWidget::caculateBigFrameDiseaseAttribute(const QRect & diseaseRect, const hnDiseaseSetInfo &diseaseSetInfo, const QString& makinfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnRoadDiseaseInfo disease;

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	disease.dWidth =std::round(  diseaseRect.width()* projectSetInfo.dRadioX *100)/100;
	disease.dLength = std::round(diseaseRect.height()* projectSetInfo.dRadioY*100)/100;
	disease.nPixelWid = diseaseRect.width();
	disease.nPixelLen = diseaseRect.height();
	if (hasSdkImageView())
	{
		const double startEncoderMile = sdkPixPointToEncoderMile(m_diseaseStartPoint);
		const double endEncoderMile = sdkPixPointToEncoderMile(m_diseaseEndPoint);
		disease.dLength = std::round(qAbs(endEncoderMile - startEncoderMile) * 100.0) / 100.0;
		disease.nPixelLen = projectSetInfo.dRadioY > 0.0
			? qRound(disease.dLength / projectSetInfo.dRadioY) : diseaseRect.height();
		disease.nPixelWid = qAbs(m_diseaseEndPoint.pixPoint.x() - m_diseaseStartPoint.pixPoint.x());
		disease.dWidth = std::round(disease.nPixelWid * projectSetInfo.dRadioX * 100.0) / 100.0;
	}
	disease.dArea = disease.dReaWidth * disease.dRealLen;

	vector<hn2dRectI> disease2dPointVector = this->generateLargeFrameHn2dRectVector(diseaseRect);
	disease.vec2dRect = disease2dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->generateLargeFrameHn3dRectVector(diseaseRect);

		//如果映射后的3d坐标数组无效，则提示用户
		if (disease3dPointVector.size() == 0)
		{
			QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败")
				, QString::fromLocal8Bit("确定"));
		}
		else
		{
			disease.vec3dRect = disease3dPointVector;
		}
	}

	hnMile firstHnMile = this->m_firstHnMile;
	disease.dRoadWidth = firstHnMile.roadWidth;
	disease.dDmi = firstHnMile.dEnclMile;
	disease.nDrawType = m_drawType;
	disease.nLevel = diseaseSetInfo.nLevel;								//病害等级
	disease.diseaseWeight = diseaseSetInfo.fWidget;						//病害权重
	disease.nRSurfaceType = firstHnMile.roadType;

	strcpy(disease.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data());

	if (hasSdkImageView())
	{
		const double startEncoderMile = sdkPixPointToEncoderMile(m_diseaseStartPoint);
		const double endEncoderMile = sdkPixPointToEncoderMile(m_diseaseEndPoint);
		disease.dDmiStart = qMin(startEncoderMile, endEncoderMile);
		disease.dDmiEnd = qMax(startEncoderMile, endEncoderMile);
		disease.dMileage = (disease.dDmiStart + disease.dDmiEnd) * 0.5;
	}
	else
	{
		disease.dMileage = this->calculateBigFrameCenterMile(diseaseRect);
		disease.dDmiStart = this->calculateBigFrameBeginMile(diseaseRect);
		disease.dDmiEnd = this->calculateBigFrameEndMile(diseaseRect);
	}

	strcpy(disease.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(disease.strDisName, diseaseSetInfo.strDiseaseTypeName);

	auto mark0 = makinfo.toLocal8Bit();
	auto mark1 = mark0.toStdString();
	strcpy(disease.strRemark, mark1.c_str());

	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	return disease;
}

hnRoadDiseaseInfo hn2dPixWidget::caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnRoadDiseaseInfo result;

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	//病害长度（线的长度）			
	double lenth = this->caculateLineDiseaseLenth(lineDiseasePoints, WIDGET_2D);
	result.dLength = lenth;

	//计算2d的坐标信息
	vector<hn2dRectI> disease2dPointVector = this->createLineDisease2dCoordVec(lineDiseasePoints);
	result.vec2dRect = disease2dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->createLineDisease3dCoordVec(lineDiseasePoints);

		//如果映射后的3d坐标数组无效，则提示用户
		if (disease3dPointVector.size() != disease2dPointVector.size())
		{
			QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败")
				, QString::fromLocal8Bit("确定"));
		}
		else
		{
			result.vec3dRect = disease3dPointVector;
		}
	}

	hnMile firstHnMile = this->m_firstHnMile;
	result.dRoadWidth = firstHnMile.roadWidth;
	result.dDmi = firstHnMile.dEnclMile;
	result.nDrawType = m_drawType;
	result.nLevel = diseaseSetInfo.nLevel;								//病害等级
	result.diseaseWeight = diseaseSetInfo.fWidget;						//病害权重
	result.nRSurfaceType = firstHnMile.roadType;

	strcpy(result.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data());

	//中心里程
	result.dMileage = this->calculateLineDiseaseCenterMile(lineDiseasePoints);
	//开始里程
	result.dDmiStart = this->calculateLineDiseaseBeginMile(lineDiseasePoints);
	//结束里程
	result.dDmiEnd = this->calculateLineDiseaseEndMile(lineDiseasePoints);

	strcpy(result.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(result.strDisName, diseaseSetInfo.strDiseaseTypeName);
	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(result);
	return result;
}

bool hn2dPixWidget::littleFrameProcess()
{
	QVector<QRect> diseaseRectsForStorage;
	QElapsedTimer totalTimer;
	QElapsedTimer stepTimer;
	totalTimer.start();
	stepTimer.start();
	auto logLittleFramePerf = [&](const char* step)
	{
		#ifdef _DEBUG
		qDebug() << "HN_LITTLE_FRAME_PERF 2d"
			<< step
			<< "ms=" << stepTimer.restart()
			<< "totalMs=" << totalTimer.elapsed()
			<< "rects=" << diseaseRectsForStorage.size()
			<< "selections=" << m_currentLittleFrameSingleSelections.size()
			<< "sdk=" << hasSdkImageView();
		#endif
	};

	if (hasSdkImageView())
	{
		if (this->littleDrawRectType)
		{
			this->commitCurrentLittleRectDrawSelection();
		}
		else if (m_currentLittleFrameSingleSelections.isEmpty() && !m_littleSingleImagePoints.isEmpty())
		{
			rebuildCurrentLittleFrameSingleSelectionsFromPoints(m_littleSingleImagePoints);
		}

		// 提交入库前才把 SDK 单图格子转成旧 QRect；SDK 预览和显示不依赖这个结果。
		this->buildCurrentLittleFrameStorageRects(diseaseRectsForStorage);
		logLittleFramePerf("buildCurrentLittleFrameStorageRects");
		if (diseaseRectsForStorage.isEmpty())
		{
			resetLittleFrameDrawState();
			return false;
		}
	}
	else
	{
		if (this->littleDrawRectType)
		{
			this->commitCurrentLittleRectDrawSelection();

			QVector<QRect> finalizedRects;
			this->appendCommittedLittleRectDrawSelection(finalizedRects);
			if (!finalizedRects.isEmpty())
			{
				this->m_tmpLittleFrameDiseaseRects = finalizedRects;
			}
		}

		if (this->m_tmpLittleFrameDiseaseRects.isEmpty())
		{
			resetLittleFrameDrawState();
			return false;
		}
		diseaseRectsForStorage = this->m_tmpLittleFrameDiseaseRects;
		logLittleFramePerf("collectLegacyLittleFrameRects");
	}

	//如果病害无效，则取消画病害
	if (!this->isTmpDiseaseRoadTypeValid(1))
	{
		logLittleFramePerf("isTmpDiseaseRoadTypeValidFailed");
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		resetLittleFrameDrawState();
		return false;
	}
	logLittleFramePerf("isTmpDiseaseRoadTypeValid");


	hnMile mile = m_firstHnMile;
	//计算病害种类
	QStringList diseaseTypeList;
	QList<QPair<QString, QString>> diseaseNameAndKey;
	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}
	logLittleFramePerf("prepareDiseaseDialog");
	if (diseaseSetInfos.isEmpty())
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("当前位置没有匹配的病害类型，请检查路面标准、路面材质和作业模式设置。"),
			QString::fromLocal8Bit("确定"));
		resetSdkDiseaseDrawingState(true, false);
		return false;
	}

	//弹出添加病害窗口
	QString diseaseTypeName;
	QString diseaseMark;
	if (!selectDiseaseTypeForDrawing(diseaseSetInfos, diseaseTypeName, diseaseMark))
	{
		#ifdef _DEBUG
		qDebug() << "HN_LITTLE_FRAME_PERF 2d dialogRejected"
			<< "totalMs=" << totalTimer.elapsed()
			<< "rects=" << diseaseRectsForStorage.size()
			<< "selections=" << m_currentLittleFrameSingleSelections.size()
			<< "sdk=" << hasSdkImageView();
		#endif
		resetSdkDiseaseDrawingState(true, false);
		return false;
	}


	//获取数据库表名和病害信息
	QString diseaseTableName;
	hnDiseaseSetInfo selectDiseaseSetInfo;
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		if (QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName) == diseaseTypeName)
		{
			selectDiseaseSetInfo = diseseSetInfo;
			diseaseTableName = QString::fromLocal8Bit(diseseSetInfo.strDBTableName);
		}
	}
	hnRoadDiseaseInfo diseaseInfo;
	logLittleFramePerf("selectDiseaseSetInfo");
	
	//计算病害属性
	diseaseInfo = this->caculateLittleFrameDiseaseAttribute(
		diseaseRectsForStorage, selectDiseaseSetInfo, diseaseMark);
	logLittleFramePerf("caculateLittleFrameDiseaseAttribute");



	//自动化模式要特殊处理 检查沉陷类病害计算
	if (!diseaseInfo.vec3dRect.empty() && true == m_isOPenDepthCaculate)
	{
		//自动化模式沉陷类病害的判别
		bool isDrawDisease = this->caculateLittleFrameDiseaseDepth(diseaseInfo,
			*this,
			&rectAlgorithm::mergeRects,
			*this,
			&drawDiseases::generateLargeFrameHn3dRectVector,
			diseaseRectsForStorage);
		logLittleFramePerf("caculateLittleFrameDiseaseDepth");
		if (false == isDrawDisease)
		{
			resetLittleFrameDrawState();

			return false;
		}
	}


	//计算病害id
	diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());
	logLittleFramePerf("getMaxID");

	if (!this->isTmpDiseaseAreaValid(diseaseInfo))
	{
		logLittleFramePerf("isTmpDiseaseAreaValidFailed");
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害面积与规范不符，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		resetLittleFrameDrawState();

		return false;
	}
	logLittleFramePerf("isTmpDiseaseAreaValid");

	if (!validateLineCameraDiseaseGeometry(diseaseInfo, this))
	{
		return false;
	}

	if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo))
	{
		logLittleFramePerf("addDiseaseFailed");
		resetLittleFrameDrawState();
		return false;
	}
	logLittleFramePerf("addDisease");
	rememberLastSdkAddedDisease(diseaseInfo);
	resetLittleFrameDrawState();
	logLittleFramePerf("done");

	return true;
}
void hn2dPixWidget::drawLittleFrameDisease(vector<hnRoadDiseaseInfo> &diseases, QImage &image)
{
	for (auto& disease : diseases)
	{
#ifdef  ALL_LITTLE_DRAW
		if (disease.vec2dRect.empty())
		{
			continue;
		}
#endif

#ifndef  ALL_LITTLE_DRAW
		if (disease.vec2dRect.empty() || disease.nDrawType == 0)
		{
			continue;
		}
#endif

		QVector<QRect> rects;
		for (const hn2dRectI &hnRect : qAsConst(disease.vec2dRect))
		{
			rects.push_back(hn2dRectToImageQtRect(hnRect));
		}

		const bool selected = selectedDiseaseId == disease.nID;
		const bool mergeSelected = isSeclectedMergeDisease(disease);

		QColor rectColor = m_diseaseDrawStyle.littleFrameRectColor;
		Qt::PenStyle penStyle = Qt::SolidLine;

		if (selected)
		{
			rectColor = m_diseaseDrawStyle.selectedRectColor;
			penStyle = Qt::DashDotDotLine;
		}
		else if (mergeSelected)
		{
			rectColor = m_diseaseDrawStyle.mergedRectColor;
		}

		const int rectWidth = selected
			? m_diseaseDrawStyle.selectedRectWidth
			: m_diseaseDrawStyle.littleFrameRectWidth;

		this->drawRectsOnImage(image, rects, diseaseImagePixels(image, rectWidth), rectColor, penStyle);

		const int fontSize = selected
			? m_diseaseDrawStyle.selectedLabelFontSize
			: m_diseaseDrawStyle.normalLabelFontSize;

		const QString diseaseInfo = buildFrameDiseaseLabel(
			disease,
			selected,
			false,
			false);

		drawDiseaseCalloutLabel(
			image,
			unitedRectOfRects(rects),
			diseaseInfo,
			diseaseImagePixels(image,fontSize),
			//fontSize,
			m_diseaseDrawStyle.labelTextColor,
			m_diseaseDrawStyle.calloutLineColor);
		}
	}

void hn2dPixWidget::littleFrameEditDisease(const QPoint & mousePoint)
{

	auto disesase = getMousePosDisease(mousePoint);
	if (!disesase.isValid())
	{
		return;
	}
	m_seclectedDiseases.clear();
	m_seclectedDiseases.append(disesase);
	this->editDisease(disesase, mousePoint);
	m_seclectedDiseases.clear();

	////获取鼠标点所在的自动化模式数组
	//QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);

	////获取鼠标位置的病害
	//for (auto disease : this->m_currentWidgetDiseases)
	//{
	//	if (disease.vec2dRect.empty())
	//	{
	//		continue;
	//	}

	//	//获取病害的二维数组
	//	std::vector<hn2dRectI>  hn2dRects = disease.vec2dRect;
	//	QVector<QRect> diseaseRects;

	//	for (hn2dRectI rect2d : qAsConst(hn2dRects))
	//	{
	//		QRect rect = this->hn2dRectToImageQtRect(rect2d);
	//		diseaseRects.push_back(rect);
	//	}
	//	//遍历病害数组，如果包含鼠标点击的点，就编辑病害
	//	for (QRect rect : qAsConst(diseaseRects))
	//	{
	//		if (rect.contains(bigImagePoint))
	//		{
	//			m_seclectedDiseases.clear();
	//			m_seclectedDiseases.append(disease);
	//			this->editDisease(disease, mousePoint);
	//			m_seclectedDiseases.clear();
	//			return;
	//		}
	//	}
	//}
}

void hn2dPixWidget::littleFrameRightButtonDragDelete(const QPoint & mousePoint)
{
	//try
	//{
	//	//获取鼠标在大图像上的坐标
	//	QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);

	//	if (m_currentWidgetDiseases.size() <= 0)
	//	{
	//		return;
	//	}
	//	//获取鼠标位置的病害
	//	for (auto& disease : currentDis)
	//	{
	//		if (disease.vec2dRect.empty())
	//		{
	//			continue;
	//		}

	//		//获取病害的二维数组
	//		std::vector<hn2dRectI>  hn2dRects = disease.vec2dRect;
	//		QVector<QRect> diseaseRects;

	//		for (hn2dRectI rect2d : qAsConst(hn2dRects))
	//		{
	//			QRect rect = this->hn2dRectToImageQtRect(rect2d);	// 病害矩形框的坐标转为在大图像上的坐标，和鼠标在大图上的坐标进行比较
	//			diseaseRects.push_back(rect);
	//		}



	//		bool project3dOpened = hnDataManager::getDataManager()->getCurrentProject()->get3DProject();
	//		vector<hn2dRectI> newVec2dRectI;
	//		vector<hn3dRectI> newVec3dRectI;
	//		//遍历病害数组，如果包含鼠标点击的点，就编辑病害
	//		for (int i = 0; i < diseaseRects.size(); i++)
	//		{
	//			// 将鼠标位置之外的自动化模式保存
	//			if (!diseaseRects[i].contains(bigImagePoint) && i < disease.vec2dRect.size())
	//			{
	//				newVec2dRectI.push_back(disease.vec2dRect[i]);
	//				if (project3dOpened)
	//				{
	//					if (disease.vec3dRect.size() > 0 && i < disease.vec3dRect.size())
	//					{

	//						newVec3dRectI.push_back(disease.vec3dRect[i]);

	//					}
	//				}

	//			}
	//		}
	//		if (newVec2dRectI.size() == disease.vec2dRect.size())
	//		{
	//			// 如果自动化模式数量没有发生变化，则不做处理
	//			continue;
	//		}


	//		disease.dArea = 0.01*newVec2dRectI.size();
	//		disease.vec2dRect = newVec2dRectI;
	//		disease.nRectCnt = newVec2dRectI.size();

	//		if (project3dOpened)
	//		{
	//			disease.vec3dRect = newVec3dRectI;
	//		}
	//		disease.n3dCnt = newVec3dRectI.size();

	//		// 重新计算病害参数，写入数据库
	//		this->updateLittleFrameDisease(disease);
	//	}
	//}
	//catch (exception* e)
	//{
	//	
	//}
	QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);

	for (const auto& srcDisease : this->m_currentWidgetDiseases)
	{
		if (srcDisease.vec2dRect.empty())
		{
			continue;
		}

		int hitIndex = -1;
		for (int i = 0; i < srcDisease.vec2dRect.size(); ++i)
		{
			QRect rect = this->hn2dRectToImageQtRect(srcDisease.vec2dRect[i]);
			if (rect.contains(bigImagePoint))
			{
				hitIndex = i;
				break;
			}
		}

		if (hitIndex < 0)
		{
			continue;
		}

		auto disease = srcDisease;
		if (hitIndex >= 0 && hitIndex < disease.vec2dRect.size())
		{
			disease.vec2dRect.erase(disease.vec2dRect.begin() + hitIndex);
		}

		if (!disease.vec3dRect.empty() && hitIndex >= 0 && hitIndex < disease.vec3dRect.size())
		{
			disease.vec3dRect.erase(disease.vec3dRect.begin() + hitIndex);
		}

		recalculateLittleFrameDiseaseAfterCellDelete(disease);

		this->updateLittleFrameDisease(disease);
		return;
	}


}

void hn2dPixWidget::littleFrameMergeDiseases(const QPoint & screenPoint)
{
	//获取选中的病害，添加到数组中
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(screenPoint, disease, hnFrameMode::LITTLE_FRAME))
		{
			if (m_seclectedDiseases.isEmpty())
			{
				m_seclectedDiseases.append(disease);
				this->update();
				break;
			}
			else
			{
				auto existDisease = m_seclectedDiseases.at(0);
				if (QString::fromLocal8Bit(existDisease.strDisName) == QString::fromLocal8Bit(disease.strDisName)
					&& existDisease.nID == disease.nID)
				{
					QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("不能选择相同的病害合并"),
						QString::fromLocal8Bit("确定"));
					return;
				}
				else
				{
					m_seclectedDiseases.append(disease);
					this->update();
					break;
				}
			}

		}
	}

	//如果等于两个,就合并两个，放到数据库中，删除原来的两个
	if (m_seclectedDiseases.size() == 2)
	{
		//获取两个病害所在图片的矩形方格，以及中间图片的矩形方格的组合
		auto acrossRects = this->caculateLittleFrameRects(m_seclectedDiseases);

		//获取两个病害的矩形方格，也就是在大图上计算病害框位置
		auto rects1 = caculateLittleFrameBigImageRects(m_seclectedDiseases.at(0));
		auto rects2 = caculateLittleFrameBigImageRects(m_seclectedDiseases.at(1));

		//合并两个病害的方格
		auto newDiseaseRects = this->mergeRects(rects1, rects2, acrossRects);

		//新病害
		auto newDisease = m_seclectedDiseases.at(0);

		//新病害有些属性继承了合并病害的第一个，有很多属性要重新计算
		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

		//病害面积
		newDisease.dArea = 0.1 * 0.1 * newDiseaseRects.size();

		//计算2d的坐标信息
		vector<hn2dRectI> disease2dPointVector = this->generateLittleFrameHn2dRectVector(newDiseaseRects);
		newDisease.vec2dRect = disease2dPointVector;

		//清空3d坐标信息
		newDisease.vec3dRect.clear();

		//如果打开了3d工程，才进行映射
		if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
		{
			//计算3d的坐标信息
			vector<hn3dRectI> disease3dPointVector = this->generateLittleFrameHn3dRectVector(newDiseaseRects);

			//如果映射后的3d坐标数组无效，则提示用户
			if (disease3dPointVector.size() == 0)
			{
				QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
					QString::fromLocal8Bit("确定"));
			}
			else
			{
				newDisease.vec3dRect = disease3dPointVector;

				//自动化模式合并病害深度计算
				if (!newDisease.vec3dRect.empty() && true == m_isOPenDepthCaculate)
				{
					//自动化模式沉陷类病害的判别
					bool isDrawDisease = this->caculateLittleFrameDiseaseDepth(newDisease,
						*this,
						&rectAlgorithm::mergeRects,
						*this,
						&drawDiseases::generateLargeFrameHn3dRectVector,
						m_tmpLittleFrameDiseaseRects);
					if (false == isDrawDisease)
					{
						//清空选中的病害数组
						m_seclectedDiseases.clear();
						return;
					}
				}
			}
		}

		//中心里程
		newDisease.dMileage = this->caculateLittleFrameMiddleMile(newDiseaseRects);
		//开始里程
		newDisease.dDmiStart = this->calculateLittleFrameBeginMile(newDiseaseRects);
		//结束里程
		newDisease.dDmiEnd = this->calculateLittleFrameEndMile(newDiseaseRects);

		//计算病害的计算面积 
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(newDisease);

		//重新设置ID
		newDisease.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->getDiseaseTable()->getMaxID(newDisease.strDiseaseTableName);

		
	

		if (!validateLineCameraDiseaseGeometry(newDisease, this))
		{
			return;
		}

		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0); 

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1); 
		if (!hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(newDisease))
		{
			QMessageBox::warning(this, QStringLiteral("\u8b66\u544a"), QStringLiteral("\u5408\u5e76\u75c5\u5bb3\u5199\u5165\u5931\u8d25\uff0c\u539f\u75c5\u5bb3\u672a\u5220\u9664\u3002"), QStringLiteral("\u786e\u5b9a"));
			m_seclectedDiseases.clear();
			return;
		}
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);
		 
		this->update();

		//清空选中的病害数组
		m_seclectedDiseases.clear();
	}
}

QVector<QRect> hn2dPixWidget::caculateLittleFrameRects(QVector<hnRoadDiseaseInfo> diseases)
{
	QVector<QRect> resultRects;

	const int diseasesCorrectSize = 2;
	if (diseasesCorrectSize != diseases.size())
	{
		return resultRects;
	}

	if (0 == diseases.size())
	{
		return resultRects;
	}

	QVector<QRect> rects;

	for (auto disease : diseases)
	{
		//计算自动化模式病害的格子，这里返回的 rects 中点的坐标是在大图上的坐标，后续合并也是在大图上进行
		rects += caculateLittleFrameBigImageRects(disease);
	}

	QMap<int, QString> pixNames;
	for (auto rect : qAsConst(rects))
	{
		QString imageName;
		QPoint p = rect.center();
		this->bigImagePointToSingleImagePoint(p, &imageName);			// 计算矩形自动化模式中心点所在的图像的名字
		int frameNum = m_pixNameMap.key(imageName);
		pixNames.insert(frameNum, imageName);
	}

	if (pixNames.isEmpty())
	{
		return resultRects;
	}

	for (auto i = pixNames.firstKey(); i <= pixNames.lastKey(); i++)
	{
		QString pixName = m_pixNameMap.value(i);
		auto imageRects = this->calculateBigImageRects(pixName);			// 单张图像上的自动化模式点坐标转到大图上
		resultRects += imageRects;
	}

	return resultRects;
}

QVector<QRect> hn2dPixWidget::caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo& disease)
{
	QVector<QRect> rects;

	if (disease.vec2dRect.empty() || 0 == disease.nDrawType)
	{
		return rects;
	}

	QRect diseaseRect;
	for (const hn2dRectI &hnRect : qAsConst(disease.vec2dRect))
	{
		// 传入包含里程信息的坐标值，转为大图上坐标值，点位于哪个图像由里程决定
		diseaseRect = hn2dRectToImageQtRect(hnRect);
		rects.push_back(diseaseRect);
	}

	return rects;
}

hnRoadDiseaseInfo hn2dPixWidget::caculateLittleFrameDiseaseAttribute(const QVector<QRect>& diseaseRects, const hnDiseaseSetInfo &diseaseSetInfo,const QString& MarkInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnRoadDiseaseInfo disease;
	QElapsedTimer attrTotalTimer;
	QElapsedTimer attrStepTimer;
	attrTotalTimer.start();
	attrStepTimer.start();

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
 
	disease.dArea = 0.1 * 0.1 *diseaseRects.size();		//病害面积
 
	//计算2d的坐标信息
	vector<hn2dRectI> disease2dPointVector = this->generateLittleFrameHn2dRectVector(diseaseRects);
	disease.vec2dRect = disease2dPointVector;
	#ifdef _DEBUG
	qDebug() << "HN_LITTLE_FRAME_PERF 2d generateLittleFrameHn2dRectVector"
		<< "rects=" << diseaseRects.size()
		<< "vec2d=" << disease.vec2dRect.size()
		<< "ms=" << attrStepTimer.restart();
	#endif

	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->generateLittleFrameHn3dRectVector(diseaseRects);
		disease.vec3dRect = disease3dPointVector;
		#ifdef _DEBUG
		qDebug() << "HN_LITTLE_FRAME_PERF 2d generateLittleFrameHn3dRectVector"
			<< "rects=" << diseaseRects.size()
			<< "vec3d=" << disease.vec3dRect.size()
			<< "ms=" << attrStepTimer.restart();
		#endif
	}
	hnMile firstHnMile = this->m_firstHnMile;
	disease.dRoadWidth = firstHnMile.roadWidth;
	disease.dDmi = firstHnMile.dEnclMile;
	disease.nDrawType = m_drawType;
	disease.nLevel = diseaseSetInfo.nLevel;
	disease.diseaseWeight = diseaseSetInfo.fWidget;		//权重
	disease.nRSurfaceType = firstHnMile.roadType;
	 
	auto mark8Bit = MarkInfo.toLocal8Bit();
	auto markStd = mark8Bit.toStdString();
	strcpy(disease.strRemark, markStd.c_str());

	strcpy(disease.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data());
	//计算病害中心里程
	disease.dMileage = this->caculateLittleFrameMiddleMile(diseaseRects);
	//开始里程
	disease.dDmiStart = this->calculateLittleFrameBeginMile(diseaseRects);
	//结束里程
	disease.dDmiEnd = this->calculateLittleFrameEndMile(diseaseRects);

	strcpy(disease.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(disease.strDisName, diseaseSetInfo.strDiseaseTypeName);

	CalculateDiseaseSize(diseaseRects,disease);

	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	#ifdef _DEBUG
	qDebug() << "HN_LITTLE_FRAME_PERF 2d caculateLittleFrameDiseaseAttributeTotal"
		<< "rects=" << diseaseRects.size()
		<< "vec2d=" << disease.vec2dRect.size()
		<< "vec3d=" << disease.vec3dRect.size()
		<< "area=" << disease.dArea
		<< "length=" << disease.dLength
		<< "width=" << disease.dWidth
		<< "totalMs=" << attrTotalTimer.elapsed();
	#endif
	return disease;
}

vector<hn2dRectI> hn2dPixWidget::generateLittleFrameHn2dRectVector(const QVector<QRect>& rects)
{
	std::vector<hn2dRectI> dstVec;
	dstVec.reserve(qMax(rects.size(), m_currentLittleFrameSingleSelections.size()));

	// SDK path: use stable image-name + single-image rect captured during preview.
	if (!m_currentLittleFrameSingleSelections.isEmpty())
	{
		for (const LittleFrameSingleRectSelection& selection : qAsConst(m_currentLittleFrameSingleSelections))
		{
			QString projectImageName = selection.pixName;
			resolveSdkImageName(selection.pixName, projectImageName);

			auto mileIter = m_pixNameHnMileMap.constFind(projectImageName);
			if (mileIter == m_pixNameHnMileMap.constEnd())
			{
				const QString fileName = QFileInfo(projectImageName).fileName();
				for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
				{
					if (QFileInfo(it.key()).fileName() == fileName)
					{
						mileIter = it;
						break;
					}
				}
			}
			if (mileIter == m_pixNameHnMileMap.constEnd())
			{
				qWarning() << "makeLittleFrameHn2dRect mile not found from sdk selection"
					<< "pixName=" << selection.pixName
					<< "resolved=" << projectImageName
					<< "singleRect=" << selection.singleRect;
				continue;
			}

			const double dmi = mileIter.value().dEnclMile;
			auto toHnPoint = [&](const QPoint& singlePoint)->hn2dPointWithMileI
			{
				int x = qBound(0, singlePoint.x(), m_pixWidth - 1);
				int y = qBound(0, singlePoint.y(), m_pixHeight - 1);
				if (m_isHMirrored)
				{
					x = m_pixWidth - 1 - x;
				}
				if (m_isVMirrored)
				{
					y = m_pixHeight - 1 - y;
				}

				hn2dPointWithMileI p;
				p.x = x;
				p.y = y;
				p.m_dmi = dmi;
				return p;
			};

			const QRect singleRect = selection.singleRect.normalized();
			hn2dRectI hnRect;
			hnRect.p0 = toHnPoint(singleRect.topLeft());
			hnRect.p1 = toHnPoint(singleRect.topRight());
			hnRect.p2 = toHnPoint(singleRect.bottomRight());
			hnRect.p3 = toHnPoint(singleRect.bottomLeft());
			dstVec.push_back(hnRect);
		}

		if (!dstVec.empty())
		{
			return dstVec;
		}
	}

	for (const QRect& rect : qAsConst(rects))
	{
		hn2dRectI hnRect; 
		if (!makeLittleFrameHn2dRect(rect,hnRect))
		{
			continue;
		}

		const int h1 = qAbs(hnRect.p2.y - hnRect.p0.y);
		const int h2 = qAbs(hnRect.p3.y - hnRect.p1.y);

		if (h1>300 ||h2>300)
		{
			qWarning() << "abnormal little rect saved"
				<< "src rect = " << rect
				<< "P0=" << hnRect.p0.x << hnRect.p0.y << hnRect.p0.m_dmi
				<< "P1=" << hnRect.p1.x << hnRect.p1.y << hnRect.p1.m_dmi
				<< "P2=" << hnRect.p2.x << hnRect.p2.y << hnRect.p2.m_dmi
				<< "P3=" << hnRect.p3.x << hnRect.p3.y << hnRect.p3.m_dmi;
		}
		dstVec.push_back(hnRect);
	}
	return dstVec;
}

vector<hn3dRectI> hn2dPixWidget::generateLittleFrameHn3dRectVector(const QVector<QRect>& rects)
{
	std::vector<hn3dRectI> dstVec;

	if (hasSdkImageView() && !m_currentLittleFrameSingleSelections.isEmpty())
	{
		auto to3dPoint = [this](const pixImagePoint& sdkPoint, hn3dPointWithMileI& out)->bool
		{
			auto dataManager = hnDataManager::getDataManager();
			if (!dataManager || !dataManager->isOpenProject() ||
				!dataManager->getCurrentProject() || !dataManager->getCurrentProject()->get3DProject())
			{
				return false;
			}

			QString projectImageName;
			if (!resolveSdkImageName(sdkPoint.pixName, projectImageName))
			{
				return false;
			}

			auto mileIter = m_pixNameHnMileMap.constFind(projectImageName);
			if (mileIter == m_pixNameHnMileMap.constEnd())
			{
				const QString fileName = QFileInfo(projectImageName).fileName();
				for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
				{
					if (QFileInfo(it.key()).fileName() == fileName)
					{
						mileIter = it;
						break;
					}
				}
			}
			if (mileIter == m_pixNameHnMileMap.constEnd())
			{
				return false;
			}

			const hnProjectSetInfo projectSetInfo = dataManager->getCurrentProject()->getCurProSetInfo();
			const double point2dMile = mileIter.value().dEnclMile +
				(m_pixHeight - qBound(0, sdkPoint.pixPoint.y(), qMax(0, m_pixHeight))) * projectSetInfo.dRadioY;
			const double target3dMile = qMax(0.0, point2dMile - dataManager->getCurrentProject()->get2d3dMileDiff());
			const double image3dHeightScale = dataManager->getCurrentProject()->get3DProject()->getImageHeightScale();
			if (image3dHeightScale <= 0.0)
			{
				return false;
			}

			const double bottom3dMile = qFloor(target3dMile / 8.0) * 8.0;
			const double offsetIn3dImage = target3dMile - bottom3dMile;
			hn2d3dCoordinates coordinates;
			int x = coordinates.single2dXToSingle3dX(qBound(0, sdkPoint.pixPoint.x(), qMax(0, m_pixWidth - 1)));
			int y = qRound(dataManager->getCurrentProject()->get3DProject()->getImagePixelHeight() - offsetIn3dImage / image3dHeightScale);
			const int image3dWidth = dataManager->getCurrentProject()->get3DProject()->getImagePixelWidth();
			const int image3dHeight = dataManager->getCurrentProject()->get3DProject()->getImagePixelHeight();
			x = qBound(0, x, qMax(0, image3dWidth - 1));
			y = qBound(0, y, qMax(0, image3dHeight - 1));

			if (dataManager->getCurrentProject()->get3DProject()->getIsHMirrored())
			{
				x = image3dWidth - 1 - x;
			}
			if (dataManager->getCurrentProject()->get3DProject()->getIsVMirrored())
			{
				y = image3dHeight - 1 - y;
			}

			out.x = x;
			out.y = y;
			out.z = 0;
			out.bottomEncoderMile = bottom3dMile;
			return true;
		};

		for (const LittleFrameSingleRectSelection& selection : qAsConst(m_currentLittleFrameSingleSelections))
		{
			const QRect singleRect = selection.singleRect.normalized();
			hn3dRectI hnRect;
			pixImagePoint p0{ selection.pixName, singleRect.topLeft() };
			pixImagePoint p1{ selection.pixName, singleRect.topRight() };
			pixImagePoint p2{ selection.pixName, singleRect.bottomRight() };
			pixImagePoint p3{ selection.pixName, singleRect.bottomLeft() };
			if (to3dPoint(p0, hnRect.p0) && to3dPoint(p1, hnRect.p1) &&
				to3dPoint(p2, hnRect.p2) && to3dPoint(p3, hnRect.p3))
			{
				dstVec.push_back(hnRect);
			}
		}

		if (!dstVec.empty())
		{
			return dstVec;
		}
	}

	hn2d3dCoordinates tool;
	for (const QRect &rect : qAsConst(rects))
	{
		hn3dPointWithMileI p = getHnPoint3dWithMileI(rect.center());
		std::vector<hn3dRectI> tmpVec = tool.get3dLittleRects(p);
		dstVec.insert(dstVec.end(), tmpVec.begin(), tmpVec.end());
	}

	return dstVec;
}
double hn2dPixWidget::caculateLittleFrameMiddleMile(const QVector<QRect>& rects)
{
	double mile;
	QRect firstRect = rects.first();
	QRect lastRect = rects.last();

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile topHnMile = getHnMileFromPoint(firstRect.topLeft());
	QString pixName;
	QPoint topPoint = this->bigImagePointToSingleImagePoint(firstRect.topLeft(), &pixName);
	double topMile = (projectSetInfo.picPixelY - topPoint.y()) * projectSetInfo.dRadioY + topHnMile.dEnclMile;

	hnMile bottomHnMile = getHnMileFromPoint(lastRect.bottomLeft());
	QPoint bottomPoint = this->bigImagePointToSingleImagePoint(lastRect.bottomLeft(), &pixName);
	double bottomMile = (projectSetInfo.picPixelY - bottomPoint.y()) * projectSetInfo.dRadioY + bottomHnMile.dEnclMile;

	mile = (topMile + bottomMile) / 2;

	return mile;
}

double hn2dPixWidget::calculateLittleFrameBeginMile(const QVector<QRect> rects)
{
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0f;
	}
	//获取自动化模式的矩形数组中，最下面那条线的y值
	const int maxY = this->findMaxY(rects);

	//获取该线所在的图片名称
	QString pixName;
	QPoint singlePoint = this->bigImagePointToSingleImagePoint(QPoint(0, maxY), &pixName);
	int singleY = singlePoint.y();

	//获取该图片的底部的里程
	auto iter = m_pixNameHnMileMap.find(pixName);
	if (iter == m_pixNameHnMileMap.end())
	{
		return 0.0f;
	}
	const double pixBottomMile = iter.value().dEnclMile;

	const double beginMile = pixBottomMile + (m_pixHeight - singleY) * m_heightScale;

	return beginMile;
}

double hn2dPixWidget::calculateLittleFrameEndMile(const QVector<QRect> rects)
{
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0f;
	}
	//获取自动化模式的矩形数组中，最上面那条线的y值
	const int maxY = this->findMinY(rects);

	//获取该线所在的图片名称
	QString pixName;
	QPoint singlePoint = this->bigImagePointToSingleImagePoint(QPoint(0, maxY), &pixName);
	int singleY = singlePoint.y();

	//获取该图片的底部的里程
	auto iter = m_pixNameHnMileMap.find(pixName);
	if (iter == m_pixNameHnMileMap.end())
	{
		return 0.0f;
	}
	const double pixBottomMile = iter.value().dEnclMile;

	const double endMile = pixBottomMile + (m_pixHeight - singleY) * m_heightScale;

	return endMile;
}

//大张图rect 转 单张图的rect	
QRect hn2dPixWidget::bigImageRectToSingleImageRect(const QRect & bigImageRect, QString * imageName)
{
	QPoint topLeft = this->bigImagePointToSingleImagePoint(bigImageRect.topLeft(), imageName);
	QPoint bottomRight = this->bigImagePointToSingleImagePoint(bigImageRect.bottomRight(), imageName);

	QRect singleImageRect(topLeft, bottomRight);

	return singleImageRect;
}

//单张图的rect 转大张图的rect
QRect hn2dPixWidget::singleImageRectToBigImageRect(const QRect & singleImageRect, const QString & imageName)
{

	//转换左上角和右下角的点
	QPoint topLeft = this->singleImagePointToBigImagePoint(singleImageRect.topLeft(), imageName);
	QPoint bottomRight = this->singleImagePointToBigImagePoint(singleImageRect.bottomRight(), imageName);

	QRect bigImageRect(topLeft, bottomRight);

	return bigImageRect;
}

QVector<QRect> hn2dPixWidget::calculateBigImageRects(const QString & imageName)
{
	//用单张图片的数组（坐标系为单张图片的自动化模式数组）直接转的
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<QRect>();
	}

	QVector<QRect> dstRects;

	for (auto singleRect : qAsConst(m_singleImageLittleFrameRects))
	{
		//把每个单张图的框，转成大张图的框
		QRect bigImageRect = singleImageRectToBigImageRect(singleRect, imageName);
		dstRects.append(bigImageRect);
	}

	return dstRects;
}

//创造单个图片的自动化模式数组
QVector<QRect> hn2dPixWidget::createSingleImageLittleFrameRect()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<QRect>();
	}

	QVector<QRect> dstRects;

	//算出每个像素代表多少米
	double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;
	double heightScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int widthSideLenth = this->caculateLittleFrameSideLenth(widthScale, rectWidth);
	int heightSideLenth = this->caculateLittleFrameSideLenth(heightScale, rectWidth);

	//图片像素宽度
	int imagePixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;

	//图片像素高度
	int imagePixelHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//横向矩形的个数
	int widthRectCount = imagePixelWidth / widthSideLenth;

	//纵向矩形的个数
	int heightRectCount = imagePixelHeight / heightSideLenth;

	//循环向目标的数据里面添加矩形
	QRect rect;					//单个自动化模式的矩形
	QPoint topLeftPoint;		//左上角的点
	QPoint bottomRightPoint;	//右下角的点

	//int tmpLenth = widthSideLenth - 1;
	for (int i = 0; i < heightRectCount; i++)
	{
		for (int j = 0; j < widthRectCount; j++)
		{
			//计算左上角的点
			topLeftPoint = QPoint(j * widthSideLenth, i * heightSideLenth);
			//计算右下角的点
			bottomRightPoint = QPoint(j * widthSideLenth + widthSideLenth, i * heightSideLenth + heightSideLenth);
			//得到矩形
			rect = QRect(topLeftPoint, bottomRightPoint);
			//插入数组
			dstRects.push_back(rect);
		}
	}

	return dstRects;
}



QVector<QRect> hn2dPixWidget::createLittleFrameRects(const QVector<pixImagePoint>& pixImagePoints)
{
	QVector<QRect> dstRects;
	QVector<QRect> bigImageRects;
	QVector<QString> pixNames;
	QSet<QString> pixNameSet;
	bool hasFrameRange = false;
	int minFrameIdx = 0;
	int maxFrameIdx = 0;

	for (const pixImagePoint& point : qAsConst(pixImagePoints))
	{
		const int frameIdx = this->m_reversePixNameMap.value(point.pixName, -1);
		if (frameIdx > 0)
		{
			if (!hasFrameRange)
			{
				minFrameIdx = frameIdx;
				maxFrameIdx = frameIdx;
				hasFrameRange = true;
			}
			else
			{
				minFrameIdx = qMin(minFrameIdx, frameIdx);
				maxFrameIdx = qMax(maxFrameIdx, frameIdx);
			}
			continue;
		}

		if (!point.pixName.isEmpty())
		{
			pixNameSet.insert(point.pixName);
		}
	}

	if (hasFrameRange)
	{
		for (int frameIdx = minFrameIdx; frameIdx <= maxFrameIdx; ++frameIdx)
		{
			const QString pixName = this->m_pixNameMap.value(frameIdx, "");
			if (!pixName.isEmpty())
			{
				pixNameSet.insert(pixName);
			}
		}
	}

	for (const QString& pixName : qAsConst(pixNameSet))
	{
		pixNames.append(pixName);
	}

	std::sort(pixNames.begin(), pixNames.end(), [this](const QString& lhs, const QString& rhs)
	{
		return this->m_reversePixNameMap.value(lhs, 2147483647) < this->m_reversePixNameMap.value(rhs, 2147483647);
	});

	for (const QString& pixName : qAsConst(pixNames))
	{
		bigImageRects.clear();
		bigImageRects = this->calculateBigImageRects(pixName);
		dstRects += bigImageRects;
	}

	return dstRects;
}

QMap<double, QString>::const_iterator hn2dPixWidget::getPreviousStakeIterator(const QMap<double, QString>&map, double curMile)
{
	if (map.isEmpty())
	{
		return map.constEnd();
	}
	auto it = map.upperBound(curMile);
	if (it == map.constBegin())
	{
		return map.constEnd();
	}
	return --it;


}

bool hn2dPixWidget::resolve2dDiseaseImageNameByMile(double encoderMile, QString& imageName) const
{
	imageName.clear();
	if (encoderMile < 0.0)
	{
		return false;
	}

	// Disease DMI is tied to the project image-mile table.  That table preserves
	// the real frame ordering; deriving an index from RoadDis can skip a frame.
	if (!m_milePixNameMap.isEmpty())
	{
		auto iter = m_milePixNameMap.upperBound(encoderMile);
		if (iter != m_milePixNameMap.constBegin())
		{
			--iter;
			if (!iter.value().isEmpty())
			{
				imageName = iter.value();
				return true;
			}
		}
	}

	// Some old projects have no valid image-mile table.  Keep index calculation
	// only as a fallback for those projects.
	if (m_imageDistanceMeters > 0.0 && !m_pixNameMap.isEmpty())
	{
		int frameIdx = qFloor(encoderMile / m_imageDistanceMeters) + 1;
		frameIdx = qBound(1, frameIdx, m_pixNameMap.size());
		const QString frameImageName = m_pixNameMap.value(frameIdx);
		if (!frameImageName.isEmpty())
		{
			imageName = frameImageName;
			return true;
		}
	}

	return false;
}
void hn2dPixWidget::drawMarkValue(QImage & image)
{ 
	//获取打标分界线 桩号
	QVector<hnCommon::hnMarkInfo>marks = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMarkVector();

	for (int i = 0; i < marks.size(); ++i)
	{
		hnCommon::hnMarkInfo curMark = marks[i];

		//路面材质
		double dmi = qRound(curMark.dEnclMile);

		if (dmi>= m_beginEncoderMile && dmi<= m_endEncoderMile)
		{
			QString markTypeStr = "";
			switch (curMark.nType)
			{
			case 0:
				markTypeStr = QStringLiteral("路面材质:");
				break;
			case 1:
				markTypeStr = QStringLiteral("路面单元:");
				break;
			case 2:
				markTypeStr = QStringLiteral("路面等级:");
				break;
			case 3:
				markTypeStr = QStringLiteral("路面标准:");
				break;
			case 4:
				markTypeStr = QStringLiteral("路面情况:");
				break;
			default:
				break;
			}
			QString typeStr = QString::fromLocal8Bit(curMark.strMark);
			//根据里程获得图片名称

			/*QPoint startPoint = this->singleImagePointToBigImagePoint(QPoint(0, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QPoint endPoint = this->singleImagePointToBigImagePoint(QPoint(m_pixWidth, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QLine line(startPoint, endPoint);
			this->drawLineOnImage(line, 20, Qt::yellow, image);*/

			//根据里程获得坐标
			hn2dPointWithMileI startPointWithMile(0, 0, dmi, 0);
			
			hn2dPointWithMileI endPointWithMile(m_pixWidth, 0, dmi, 0);

			QPoint startPoint = this->hn2dPointToImageQtPoint(startPointWithMile);
			 
			QPoint endPoint = this->hn2dPointToImageQtPoint(endPointWithMile);
			

			if (m_isVMirrored)
			{
				if ((int)dmi % 2 == 0)
				{
					//startPoint.setY(startPoint.y() + m_pixHeight);

					//endPoint.setY(endPoint.y() + m_pixHeight);
				}
				else
				{
					startPoint.setY(startPoint.y() - m_pixHeight);

					endPoint.setY(endPoint.y() - m_pixHeight);
				}
				
			}
			else
			{

				if ((int)dmi % 2 == 0)
				{
					startPoint.setY(startPoint.y() + m_pixHeight);

					endPoint.setY(endPoint.y() + m_pixHeight);
				}
				else
				{

				}
			 
			}
			 
			QLine line(startPoint, endPoint);
			QPainter painter(&image);
			QColor lineColor = curMark.nType == 0 ? QColor(0, 170, 255) : QColor(120, 120, 120);
			QColor fillColor = lineColor;
			fillColor.setAlpha(curMark.nType == 0 ? 50 : 25);
			const int bandHalfHeight = curMark.nType == 0 ? 24 : 12;
			const QRect bandRect(QPoint(0, startPoint.y() - bandHalfHeight), QPoint(m_pixWidth, startPoint.y() + bandHalfHeight));
			painter.fillRect(bandRect.normalized(), fillColor);

			QPen pen(lineColor);
			pen.setWidth(curMark.nType == 0 ? 6 : 3);
			painter.setPen(pen);
			painter.drawLine(line);

			QFont font = painter.font();
			font.setBold(true);
			font.setPixelSize(curMark.nType == 0 ? 54 : 40);
			painter.setFont(font);
			QString diseaseInfo = markTypeStr + MyCommonMethods::convertMileToString(curMark.dTrueMile) + "_" + typeStr;
			const int textX = qBound(10, qMin(startPoint.x(), endPoint.x()) + 30, qMax(10, m_pixWidth - 1200));
			const int textY = startPoint.y() - bandHalfHeight - 8;
			painter.drawText(QPoint(textX, textY), diseaseInfo);
		}
	
	}
}

bool hn2dPixWidget::isTmpDiseaseAreaValid(const hnRoadDiseaseInfo & disease)
{

	if (strcmp( disease.strRoadStandard,"低等级农村公路") == 0)
	{
		if (strcmp(disease.strDiseaseTableName,"DisLG")==0 ||
			strcmp(disease.strDiseaseTableName, "DisSS") == 0)
		{
			if (disease.dArea <20)
			{
				return false;
			}
		}
	}
	return true;
}



hnRoadDiseaseInfo hn2dPixWidget::getMousePosDisease(const QPoint & mousePoint)
{
	if (m_frameMode ==  FrameMode::LITTLE_FRAME)
	{
		//获取鼠标点所在的自动化模式数组
		QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);

		//获取鼠标位置的病害
		for (auto disease : this->m_currentWidgetDiseases)
		{
			if (disease.vec2dRect.empty())
			{
				continue;
			}

			//获取病害的二维数组
			std::vector<hn2dRectI>  hn2dRects = disease.vec2dRect;
			QVector<QRect> diseaseRects;

		
			for (hn2dRectI rect2d : qAsConst(hn2dRects))
			{
				QRect rect = this->hn2dRectToImageQtRect(rect2d);
				diseaseRects.push_back(rect);
			}
		 
		
		
			//遍历病害数组，如果包含鼠标点击的点，就编辑病害
			for (QRect rect : qAsConst(diseaseRects))
			{
				if (rect.contains(bigImagePoint))
				{
					return disease; 
				}
			}
			//最大外接矩形
			QRect boundingRect = diseaseRects.first();
			for (const QRect& rect : diseaseRects)
			{
				boundingRect = boundingRect.united(rect);
			}
			if (boundingRect.contains(bigImagePoint))
			{
				return disease;
			} 
		}
	}
	else
	{
		for (auto disease : this->m_currentWidgetDiseases)
		{
			if (disease.vec2dRect.empty())
			{
				continue;
			}
			hn2dRectI  hnRect = disease.vec2dRect.at(0);

			QRect diseaseRect = this->hn2dRectToImageQtRect(hnRect);

			QPoint imagePoint = this->screenPointToBigImagePoint(mousePoint);
			if (diseaseRect.contains(imagePoint))
			{
				return disease;
			}
		}
	}

	return hnRoadDiseaseInfo();
}

void hn2dPixWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	bool isvalid = isValidArea(event);
	if (!isvalid)
	{
		return;
	}

	if (this->m_frameMode == FrameMode::BIG_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:
			 
				break;
			case hnWorkMode::DELETE_MODE:
		 
				break;
			case hnWorkMode::EDIT_MODE:
			 
				break;
			case hnWorkMode::MOVE:
				break;
			case hnWorkMode::MERGE:
				 
				break;
			case hnWorkMode::GET_MILE:
				break;
			case hnWorkMode::ADD_CTRL_POINT:
				break;
			default:
				break;
			}
		}
		if (event->button() == Qt::RightButton)
		{
			this->bigFrameEditProcess(event->pos());
		}
	} 


	if (this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:

				break;
			case hnWorkMode::DELETE_MODE:

				break;
			case hnWorkMode::EDIT_MODE:

				break;
			case hnWorkMode::MOVE:
				break;
			case hnWorkMode::MERGE:

				break;
			case hnWorkMode::GET_MILE:
				break;
			case hnWorkMode::ADD_CTRL_POINT:
				break;
			default:
				break;
			}
		}
		if (event->button() == Qt::RightButton)
		{
			++m_pendingRightClcikDeleteSerial;
			m_pendingRightClickDeletePoint = QPoint(-1,-1);
			m_isRightDeleteMouseDown = false;
			m_RightDeleteMousePoint = QPoint(-1, -1);
			this->selectDisease(event->pos());
			event->accept();
			return;
		}
	}
}

bool hn2dPixWidget::adjustSdkDiseasePointToValidArea(pixImagePoint& point, bool clampToArea) const
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project || !project->isLineCameraProject())
	{
		return true;
	}
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	if (!info.validAreaConfigured || info.imageWidth <= 0)
	{
		return false;
	}
	const int displayedX = qBound(0, point.pixPoint.x(), info.imageWidth);
	const int sourceX = m_isHMirrored ? info.imageWidth - displayedX : displayedX;
	if (sourceX >= info.leftPixel && sourceX <= info.rightPixel)
	{
		return true;
	}
	if (!clampToArea)
	{
		QToolTip::showText(QCursor::pos(), QStringLiteral("线阵病害只能在有效道路区域内绘制。"));
		return false;
	}
	const int clampedSourceX = qBound(info.leftPixel, sourceX, info.rightPixel);
	point.pixPoint.setX(m_isHMirrored ? info.imageWidth - clampedSourceX : clampedSourceX);
	return true;
}

bool hn2dPixWidget::validateDiseaseGeometryWithinValidArea(const hnRoadDiseaseInfo& disease) const
{
	return validateLineCameraDiseaseGeometry(disease, const_cast<hn2dPixWidget*>(this));
}
void hn2dPixWidget::clearLineCameraAreaGuide()
{
	QGraphicsScene* scene = m_sdkImageView ? m_sdkImageView->scene() : nullptr;
	auto removeItem = [scene](QGraphicsItem*& item)
	{
		if (!item) return;
		if (scene && item->scene() == scene) scene->removeItem(item);
		delete item;
		item = nullptr;
	};

	QGraphicsItem* leftShade = m_lineCameraPersistentLeftShade;
	QGraphicsItem* rightShade = m_lineCameraPersistentRightShade;
	QGraphicsItem* leftBoundary = m_lineCameraPersistentLeftBoundary;
	QGraphicsItem* rightBoundary = m_lineCameraPersistentRightBoundary;
	removeItem(leftShade);
	removeItem(rightShade);
	removeItem(leftBoundary);
	removeItem(rightBoundary);
	m_lineCameraPersistentLeftShade = nullptr;
	m_lineCameraPersistentRightShade = nullptr;
	m_lineCameraPersistentLeftBoundary = nullptr;
	m_lineCameraPersistentRightBoundary = nullptr;
}

void hn2dPixWidget::refreshLineCameraAreaGuide()
{
	clearLineCameraAreaGuide();
	if (m_lineCameraAreaAdjusting || !m_sdkImageView || !m_sdkImageView->scene()) return;

	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project || !project->isLineCameraProject()) return;
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	if (!info.validAreaConfigured || info.imageWidth <= 0) return;

	QGraphicsScene* scene = m_sdkImageView->scene();
	const QRectF sceneRect = scene->sceneRect();
	if (sceneRect.isEmpty()) return;

	const qreal sourceLeft = m_isHMirrored ? info.imageWidth - info.leftPixel : info.leftPixel;
	const qreal sourceRight = m_isHMirrored ? info.imageWidth - info.rightPixel : info.rightPixel;
	const qreal validLeft = qMin(sourceLeft, sourceRight);
	const qreal validRight = qMax(sourceLeft, sourceRight);
	const QBrush invalidAreaBrush(QColor(12, 18, 26, 135));

	m_lineCameraPersistentLeftShade = scene->addRect(
		QRectF(sceneRect.left(), sceneRect.top(), qMax(0.0, validLeft - sceneRect.left()), sceneRect.height()),
		Qt::NoPen, invalidAreaBrush);
	m_lineCameraPersistentRightShade = scene->addRect(
		QRectF(validRight, sceneRect.top(), qMax(0.0, sceneRect.right() - validRight), sceneRect.height()),
		Qt::NoPen, invalidAreaBrush);

	QPen leftPen(QColor(0, 235, 255, 235), 2.5, Qt::DashLine);
	QPen rightPen(QColor(255, 205, 45, 235), 2.5, Qt::DashLine);
	leftPen.setCosmetic(true);
	rightPen.setCosmetic(true);
	m_lineCameraPersistentLeftBoundary = scene->addLine(
		QLineF(validLeft, sceneRect.top(), validLeft, sceneRect.bottom()), leftPen);
	m_lineCameraPersistentRightBoundary = scene->addLine(
		QLineF(validRight, sceneRect.top(), validRight, sceneRect.bottom()), rightPen);

	QGraphicsItem* items[] = {
		m_lineCameraPersistentLeftShade, m_lineCameraPersistentRightShade,
		m_lineCameraPersistentLeftBoundary, m_lineCameraPersistentRightBoundary
	};
	for (QGraphicsItem* item : items)
	{
		item->setAcceptedMouseButtons(Qt::NoButton);
		item->setZValue(item == m_lineCameraPersistentLeftShade || item == m_lineCameraPersistentRightShade
			? 10000.0 : 10001.0);
	}
}

bool hn2dPixWidget::startLineCameraAreaAdjustment()
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (m_lineCameraAreaAdjusting || !project || !project->isLineCameraProject() ||
		!m_sdkImageView || !m_sdkImageView->scene())
	{
		return false;
	}
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	if (!info.validAreaConfigured || info.imageWidth <= 0)
	{
		return false;
	}

	slot_cancelDrawDiseases();
	resetSdkDiseaseDrawingState(true, true);
	m_workMode = WorkMode::NO_MODE;
	m_lineCameraAreaAdjusting = true;
	if (m_lineCameraPersistentLeftShade) m_lineCameraPersistentLeftShade->setVisible(false);
	if (m_lineCameraPersistentRightShade) m_lineCameraPersistentRightShade->setVisible(false);
	if (m_lineCameraPersistentLeftBoundary) m_lineCameraPersistentLeftBoundary->setVisible(false);
	if (m_lineCameraPersistentRightBoundary) m_lineCameraPersistentRightBoundary->setVisible(false);

	QGraphicsScene* scene = m_sdkImageView->scene();
	const QRectF sceneRect = scene->sceneRect();
	const qreal visualLeft = m_isHMirrored ? info.imageWidth - info.leftPixel : info.leftPixel;
	const qreal visualRight = m_isHMirrored ? info.imageWidth - info.rightPixel : info.rightPixel;

	m_lineCameraLeftShade = scene->addRect(QRectF(), Qt::NoPen, QColor(0, 0, 0, 145));
	m_lineCameraRightShade = scene->addRect(QRectF(), Qt::NoPen, QColor(0, 0, 0, 145));
	m_lineCameraLeftShade->setZValue(99998.0);
	m_lineCameraRightShade->setZValue(99998.0);
	m_lineCameraLeftBoundary = new hnLineCameraBoundaryItem(QColor(0, 255, 255));
	m_lineCameraRightBoundary = new hnLineCameraBoundaryItem(QColor(255, 215, 0));
	scene->addItem(m_lineCameraLeftBoundary);
	scene->addItem(m_lineCameraRightBoundary);
	m_lineCameraLeftBoundary->setLine(0.0, sceneRect.top(), 0.0, sceneRect.bottom());
	m_lineCameraRightBoundary->setLine(0.0, sceneRect.top(), 0.0, sceneRect.bottom());
	if (m_isHMirrored)
	{
		m_lineCameraLeftBoundary->setHorizontalRange(visualRight + 1.0, info.imageWidth);
		m_lineCameraRightBoundary->setHorizontalRange(0.0, visualLeft - 1.0);
	}
	else
	{
		m_lineCameraLeftBoundary->setHorizontalRange(0.0, visualRight - 1.0);
		m_lineCameraRightBoundary->setHorizontalRange(visualLeft + 1.0, info.imageWidth);
	}
	m_lineCameraLeftBoundary->setPos(visualLeft, 0.0);
	m_lineCameraRightBoundary->setPos(visualRight, 0.0);
	m_lineCameraLeftBoundary->moved = [this]() { updateLineCameraAreaOverlay(); };
	m_lineCameraRightBoundary->moved = [this]() { updateLineCameraAreaOverlay(); };

	m_lineCameraAreaPanel = new QFrame(this);
	m_lineCameraAreaPanel->setStyleSheet(QStringLiteral(
		"QFrame{background:rgba(25,25,25,225);border:1px solid #aaaaaa;border-radius:5px;}"
		"QLabel{color:white;} QLineEdit{background:white;color:#202020;border:1px solid #888;padding:3px;}"));
	QVBoxLayout* panelLayout = new QVBoxLayout(m_lineCameraAreaPanel);
	panelLayout->setContentsMargins(10, 7, 10, 7);
	panelLayout->setSpacing(6);
	m_lineCameraAreaSummary = new QLabel(m_lineCameraAreaPanel);
	panelLayout->addWidget(m_lineCameraAreaSummary);

	m_lineCameraLeftPixelEdit = new QLineEdit(m_lineCameraAreaPanel);
	m_lineCameraRightPixelEdit = new QLineEdit(m_lineCameraAreaPanel);
	m_lineCameraRoadWidthEdit = new QLineEdit(m_lineCameraAreaPanel);
	m_lineCameraLeftPixelEdit->setValidator(new QIntValidator(0, info.imageWidth, m_lineCameraLeftPixelEdit));
	m_lineCameraRightPixelEdit->setValidator(new QIntValidator(0, info.imageWidth, m_lineCameraRightPixelEdit));
	QDoubleValidator* widthValidator = new QDoubleValidator(0.0,
		info.imageWidth * info.meterPerPixelWidth(), 6, m_lineCameraRoadWidthEdit);
	widthValidator->setNotation(QDoubleValidator::StandardNotation);
	m_lineCameraRoadWidthEdit->setValidator(widthValidator);
	m_lineCameraLeftPixelEdit->setFixedWidth(90);
	m_lineCameraRightPixelEdit->setFixedWidth(90);
	m_lineCameraRoadWidthEdit->setFixedWidth(105);
	connect(m_lineCameraLeftPixelEdit, &QLineEdit::textEdited, this,
		[this]() { m_lineCameraPendingInputMode = 1; });
	connect(m_lineCameraRightPixelEdit, &QLineEdit::textEdited, this,
		[this]() { m_lineCameraPendingInputMode = 1; });
	connect(m_lineCameraRoadWidthEdit, &QLineEdit::textEdited, this,
		[this]() { m_lineCameraPendingInputMode = 2; });
	connect(m_lineCameraLeftPixelEdit, &QLineEdit::returnPressed, this,
		[this]() { applyLineCameraPixelInputs(); });
	connect(m_lineCameraRightPixelEdit, &QLineEdit::returnPressed, this,
		[this]() { applyLineCameraPixelInputs(); });
	connect(m_lineCameraRoadWidthEdit, &QLineEdit::returnPressed, this,
		[this]() { applyLineCameraRoadWidthInput(); });

	QPushButton* applyPixelsButton = new QPushButton(QStringLiteral("\u5e94\u7528\u50cf\u7d20\u8fb9\u754c"), m_lineCameraAreaPanel);
	QPushButton* applyWidthButton = new QPushButton(QStringLiteral("\u6309\u5bbd\u5ea6\u5c45\u4e2d"), m_lineCameraAreaPanel);
	connect(applyPixelsButton, &QPushButton::clicked, this, [this]() { applyLineCameraPixelInputs(); });
	connect(applyWidthButton, &QPushButton::clicked, this, [this]() { applyLineCameraRoadWidthInput(); });
	QHBoxLayout* inputLayout = new QHBoxLayout();
	inputLayout->addWidget(new QLabel(QStringLiteral("\u5de6(px)"), m_lineCameraAreaPanel));
	inputLayout->addWidget(m_lineCameraLeftPixelEdit);
	inputLayout->addWidget(new QLabel(QStringLiteral("\u53f3(px)"), m_lineCameraAreaPanel));
	inputLayout->addWidget(m_lineCameraRightPixelEdit);
	inputLayout->addWidget(applyPixelsButton);
	inputLayout->addSpacing(12);
	inputLayout->addWidget(new QLabel(QStringLiteral("\u9053\u8def\u5bbd\u5ea6(m)"), m_lineCameraAreaPanel));
	inputLayout->addWidget(m_lineCameraRoadWidthEdit);
	inputLayout->addWidget(applyWidthButton);
	panelLayout->addLayout(inputLayout);

	QPushButton* resetButton = new QPushButton(QStringLiteral("\u6062\u590d\u6574\u56fe"), m_lineCameraAreaPanel);
	QPushButton* saveButton = new QPushButton(QStringLiteral("\u4fdd\u5b58"), m_lineCameraAreaPanel);
	QPushButton* cancelButton = new QPushButton(QStringLiteral("\u53d6\u6d88"), m_lineCameraAreaPanel);
	QHBoxLayout* actionLayout = new QHBoxLayout();
	actionLayout->addStretch();
	actionLayout->addWidget(resetButton);
	actionLayout->addWidget(saveButton);
	actionLayout->addWidget(cancelButton);
	panelLayout->addLayout(actionLayout);
	connect(resetButton, &QPushButton::clicked, this,
		[this, info]() { setLineCameraAdjustmentBounds(0, info.imageWidth); });
	connect(saveButton, &QPushButton::clicked, this, [this]()
	{
		if (applyLineCameraPendingInput()) finishLineCameraAreaAdjustment(true);
	});
	connect(cancelButton, &QPushButton::clicked, this, [this]() { finishLineCameraAreaAdjustment(false); });
	m_lineCameraAreaPanel->adjustSize();
	m_lineCameraAreaPanel->move(qMax(8, width() - m_lineCameraAreaPanel->width() - 12), 12);
	m_lineCameraAreaPanel->show();
	m_lineCameraAreaPanel->raise();
	updateLineCameraAreaOverlay();
	emit signal_lineCameraAreaAdjustmentStateChanged(true);
	return true;
}

void hn2dPixWidget::setLineCameraAdjustmentBounds(int leftPixel, int rightPixel)
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project || !m_lineCameraLeftBoundary || !m_lineCameraRightBoundary) return;
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	if (leftPixel < 0 || leftPixel >= rightPixel || rightPixel > info.imageWidth) return;

	m_lineCameraLeftBoundary->setHorizontalRange(0.0, info.imageWidth);
	m_lineCameraRightBoundary->setHorizontalRange(0.0, info.imageWidth);
	m_lineCameraLeftBoundary->setPos(m_isHMirrored ? info.imageWidth - leftPixel : leftPixel, 0.0);
	m_lineCameraRightBoundary->setPos(m_isHMirrored ? info.imageWidth - rightPixel : rightPixel, 0.0);
	m_lineCameraPendingInputMode = 0;
	updateLineCameraAreaOverlay();
}

bool hn2dPixWidget::applyLineCameraPixelInputs()
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project) return false;
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	bool leftOk = false;
	bool rightOk = false;
	const int left = m_lineCameraLeftPixelEdit
		? m_lineCameraLeftPixelEdit->text().trimmed().toInt(&leftOk) : -1;
	const int right = m_lineCameraRightPixelEdit
		? m_lineCameraRightPixelEdit->text().trimmed().toInt(&rightOk) : -1;
	if (!leftOk || !rightOk || left < 0 || left >= right || right > info.imageWidth)
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u50cf\u7d20\u8fb9\u754c\u5fc5\u987b\u6ee1\u8db3\uff1a0 <= \u5de6\u8fb9\u754c < \u53f3\u8fb9\u754c <= %1\u3002")
			.arg(info.imageWidth));
		return false;
	}
	setLineCameraAdjustmentBounds(left, right);
	return true;
}

bool hn2dPixWidget::applyLineCameraRoadWidthInput()
{
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project) return false;
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	bool widthOk = false;
	const double requestedWidth = m_lineCameraRoadWidthEdit
		? m_lineCameraRoadWidthEdit->text().trimmed().toDouble(&widthOk) : 0.0;
	const double metersPerPixel = info.meterPerPixelWidth();
	const double fullImageWidth = info.imageWidth * metersPerPixel;
	if (!widthOk || requestedWidth <= 0.0 || metersPerPixel <= 0.0 || requestedWidth > fullImageWidth + 1e-9)
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u9053\u8def\u5bbd\u5ea6\u5fc5\u987b\u5927\u4e8e 0\uff0c\u4e14\u4e0d\u80fd\u8d85\u8fc7\u6574\u5e45\u56fe\u50cf\u53ef\u8868\u793a\u7684 %1 m\u3002")
			.arg(fullImageWidth, 0, 'f', 5));
		return false;
	}
	int left = 0;
	int right = 0;
	if (!info.centeredBoundsForRoadWidth(requestedWidth, left, right))
	{
		QMessageBox::warning(this, QStringLiteral("\u8f93\u5165\u65e0\u6548"),
			QStringLiteral("\u8be5\u5bbd\u5ea6\u65e0\u6cd5\u6362\u7b97\u4e3a\u5408\u6cd5\u7684\u5c45\u4e2d\u50cf\u7d20\u8fb9\u754c\u3002"));
		return false;
	}
	setLineCameraAdjustmentBounds(left, right);
	return true;
}

bool hn2dPixWidget::applyLineCameraPendingInput()
{
	if (m_lineCameraPendingInputMode == 1) return applyLineCameraPixelInputs();
	if (m_lineCameraPendingInputMode == 2) return applyLineCameraRoadWidthInput();
	return true;
}

void hn2dPixWidget::updateLineCameraAreaOverlay()
{
	if (!m_lineCameraAreaAdjusting || !m_lineCameraLeftBoundary || !m_lineCameraRightBoundary ||
		!m_sdkImageView || !m_sdkImageView->scene())
	{
		return;
	}
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (!project)
	{
		return;
	}
	const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
	const int left = qBound(0, qRound(m_isHMirrored
		? info.imageWidth - m_lineCameraLeftBoundary->pos().x()
		: m_lineCameraLeftBoundary->pos().x()), info.imageWidth);
	const int right = qBound(0, qRound(m_isHMirrored
		? info.imageWidth - m_lineCameraRightBoundary->pos().x()
		: m_lineCameraRightBoundary->pos().x()), info.imageWidth);
	const qreal leftVisual = m_lineCameraLeftBoundary->pos().x();
	const qreal rightVisual = m_lineCameraRightBoundary->pos().x();
	if (m_isHMirrored)
	{
		m_lineCameraLeftBoundary->setHorizontalRange(rightVisual + 1.0, info.imageWidth);
		m_lineCameraRightBoundary->setHorizontalRange(0.0, leftVisual - 1.0);
	}
	else
	{
		m_lineCameraLeftBoundary->setHorizontalRange(0.0, rightVisual - 1.0);
		m_lineCameraRightBoundary->setHorizontalRange(leftVisual + 1.0, info.imageWidth);
	}
	const QRectF sceneRect = m_sdkImageView->scene()->sceneRect();
	const qreal validVisualLeft = qMin(leftVisual, rightVisual);
	const qreal validVisualRight = qMax(leftVisual, rightVisual);
	m_lineCameraLeftShade->setRect(sceneRect.left(), sceneRect.top(),
		qMax(0.0, validVisualLeft - sceneRect.left()), sceneRect.height());
	m_lineCameraRightShade->setRect(validVisualRight, sceneRect.top(),
		qMax(0.0, sceneRect.right() - validVisualRight), sceneRect.height());
	const int validWidth = qMax(0, right - left);
	const double roadWidth = validWidth * info.meterPerPixelWidth();
	m_lineCameraPendingInputMode = 0;
	if (m_lineCameraLeftPixelEdit && !m_lineCameraLeftPixelEdit->hasFocus())
		m_lineCameraLeftPixelEdit->setText(QString::number(left));
	if (m_lineCameraRightPixelEdit && !m_lineCameraRightPixelEdit->hasFocus())
		m_lineCameraRightPixelEdit->setText(QString::number(right));
	if (m_lineCameraRoadWidthEdit && !m_lineCameraRoadWidthEdit->hasFocus())
		m_lineCameraRoadWidthEdit->setText(QString::number(roadWidth, 'f', 5));
	if (m_lineCameraAreaSummary)
	{
		m_lineCameraAreaSummary->setText(QStringLiteral("左 %1 px  右 %2 px  有效 %3 px  道路 %4 m")
			.arg(left).arg(right).arg(validWidth)
			.arg(validWidth * info.meterPerPixelWidth(), 0, 'f', 5));
	}
}

int hn2dPixWidget::diseasesOutsideLineCameraArea(int leftPixel, int rightPixel) const
{
	int affected = 0;
	auto service = hnApp::hnDataManager::getDataManager()->getDiseaseService();
	if (!service)
	{
		return affected;
	}
	const QVector<hnCommon::hnRoadDiseaseInfo> diseases = service->getAllRoadDiseases();
	for (const hnCommon::hnRoadDiseaseInfo& disease : diseases)
	{
		bool outside = false;
		for (const hn2dRectI& rect : disease.vec2dRect)
		{
			const int xs[] = { rect.p0.x, rect.p1.x, rect.p2.x, rect.p3.x };
			const int pointCount = disease.nDrawType == 3 ? 1 : 4;
			for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
			{
				const int x = xs[pointIndex];
				if (x < leftPixel || x > rightPixel)
				{
					outside = true;
					break;
				}
			}
			if (outside) break;
		}
		if (outside) ++affected;
	}	return affected;
}

void hn2dPixWidget::finishLineCameraAreaAdjustment(bool saveChanges)
{
	if (!m_lineCameraAreaAdjusting)
	{
		return;
	}
	auto project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (saveChanges && project && m_lineCameraLeftBoundary && m_lineCameraRightBoundary)
	{
		const hnPro::hnLineCameraInfo& info = project->getLineCameraInfo();
		const int left = qBound(0, qRound(m_isHMirrored
			? info.imageWidth - m_lineCameraLeftBoundary->pos().x()
			: m_lineCameraLeftBoundary->pos().x()), info.imageWidth);
		const int right = qBound(0, qRound(m_isHMirrored
			? info.imageWidth - m_lineCameraRightBoundary->pos().x()
			: m_lineCameraRightBoundary->pos().x()), info.imageWidth);
		const int affected = diseasesOutsideLineCameraArea(left, right);
		if (affected > 0)
		{
			QMessageBox::warning(this, QStringLiteral("无法保存有效区域"),
				QStringLiteral("新边界会排除 %1 个已有病害。请扩大边界，或先处理这些病害。 ").arg(affected));
			return;
		}
		QString errorMessage;
		if (!project->setLineCameraValidArea(left, right, &errorMessage))
		{
			QMessageBox::critical(this, QStringLiteral("保存失败"), errorMessage);
			return;
		}
	}

	QGraphicsScene* scene = m_sdkImageView ? m_sdkImageView->scene() : nullptr;
	auto removeItem = [scene](QGraphicsItem*& item)
	{
		if (!item) return;
		if (scene) scene->removeItem(item);
		delete item;
		item = nullptr;
	};
	QGraphicsItem* leftBoundary = m_lineCameraLeftBoundary;
	QGraphicsItem* rightBoundary = m_lineCameraRightBoundary;
	QGraphicsItem* leftShade = m_lineCameraLeftShade;
	QGraphicsItem* rightShade = m_lineCameraRightShade;
	removeItem(leftBoundary);
	removeItem(rightBoundary);
	removeItem(leftShade);
	removeItem(rightShade);
	m_lineCameraLeftBoundary = nullptr;
	m_lineCameraRightBoundary = nullptr;
	m_lineCameraLeftShade = nullptr;
	m_lineCameraRightShade = nullptr;
	delete m_lineCameraAreaPanel;
	m_lineCameraAreaPanel = nullptr;
	m_lineCameraAreaSummary = nullptr;
	m_lineCameraLeftPixelEdit = nullptr;
	m_lineCameraRightPixelEdit = nullptr;
	m_lineCameraRoadWidthEdit = nullptr;
	m_lineCameraPendingInputMode = 0;
	m_lineCameraAreaAdjusting = false;
	refreshLineCameraAreaGuide();
	if (saveChanges)
	{
		m_widthScale = project->getCurProSetInfo().dRadioX;
		refreshSdkDiseaseLayer();
		update();
	}
	emit signal_lineCameraAreaAdjustmentStateChanged(false);
}
bool hn2dPixWidget::isValidArea(QMouseEvent * event)
{

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return false;
	}

	if (m_pixNameMap.empty())
	{
		return false;
	}
	QPoint bigImagePoint = this->screenPointToBigImagePoint(event->pos());
	auto currentProject = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	if (currentProject && currentProject->isLineCameraProject() &&
		(m_workMode == WorkMode::ADD_MODE || m_workMode == WorkMode::EDIT_MODE || m_workMode == WorkMode::MOVE))
	{
		const hnPro::hnLineCameraInfo& info = currentProject->getLineCameraInfo();
		const int displayedX = qBound(0, bigImagePoint.x(), info.imageWidth);
		const int sourceX = m_isHMirrored ? info.imageWidth - displayedX : displayedX;
		if (!info.containsPixelX(sourceX))
		{
			QToolTip::showText(event->globalPos(), QStringLiteral("线阵病害只能在有效道路区域内操作。"));
			return false;
		}
	}
	this->m_firstHnMile = this->getHnMileFromPoint(bigImagePoint);

	//判断当前桩号 是否在用户规定区间内
	auto setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	if (setting.dUserBegMile != -1)
	{
		if (setting.nLineType * setting.dUserBegMile > m_firstHnMile.dTrueMile * setting.nLineType || m_firstHnMile.dTrueMile > setting.nLineType * setting.dUserEndMile)
		{
			QMessageBox::critical(this, "error", QStringLiteral("多工程已设置的有效病害绘制区间为\n【") + QString::number(setting.dUserBegMile)
				+ "~" + QString::number(setting.dUserEndMile) + QStringLiteral("】请跳转到有效区间继续操作或清空多工程桩号配置！"));
			return false;
		}
	}
	return true;
}

void hn2dPixWidget::selectDisease(const QPoint & mousePoint)
{
	//选中病害 病害列表选中
	auto disease = getMousePosDisease(mousePoint);
	if (disease.isValid())
	{
		
		selectedDiseaseId = disease.nID;
		m_seclectedDiseases.clear();
		m_seclectedDiseases.append(disease);
	
		emit	signal_selectDisease(disease);
		update();
	}
}

bool hn2dPixWidget::sdkCommitBigFrameDisease()
{
	return drawBigFrameProcess();
}

bool hn2dPixWidget::sdkCommitLittleFrameDisease()
{
	return littleFrameProcess();
}

QVector<QRect> hn2dPixWidget::sdkCreateLittleFrameRects(const QVector<pixImagePoint>& pixImagePoints)
{
	// Deprecated: SDK drawing now uses GridSelectionTool directly; keep this only for legacy paths.
	return createLittleFrameRects(pixImagePoints);
}

QString hn2dPixWidget::sdkStatusInfoFromWidgetPoint(const QPoint& widgetPoint)
{
	return generateStatusInfo(widgetPoint);
}

QString hn2dPixWidget::sdkStatusInfoFromContext(const hn2d3dPixBaseWidget::SdkStatusContext& context)
{
	if (!context.valid || !hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QString();
	}

	hnCommon::hnProjectSetInfo projectSetInfo =
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnMile currentPointMile;
	auto iter = m_pixNameHnMileMap.constFind(context.imageName);
	if (iter != m_pixNameHnMileMap.constEnd())
	{
		currentPointMile = iter.value();
	}
	else
	{
		for (auto it = m_pixNameHnMileMap.constBegin(); it != m_pixNameHnMileMap.constEnd(); ++it)
		{
			if (it.key().contains(context.imageName) || context.imageName.contains(it.key()))
			{
				currentPointMile = it.value();
				break;
			}
		}
	}

	const double routeYInImage = context.routeImagePoint.y() - (context.imageIndex - 1) * m_pixHeight;
	const double localY = qBound(0.0, m_pixHeight - routeYInImage, m_pixHeight * 1.0);
	const double bottomEncoderMile = currentPointMile.dEnclMile;
	const double encoderMile = bottomEncoderMile + (m_pixHeight - localY) * projectSetInfo.dRadioY;
	const double trueMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(encoderMile);
    auto statusProject = hnDataManager::getDataManager()->getCurrentProject();
    const double roadWidth = statusProject ? statusProject->effectiveRoadWidth() : qMax(0.0, projectSetInfo.dRoadWidth);
    const double rawXMile = statusProject && statusProject->isLineCameraProject()
        ? statusProject->getLineCameraInfo().roadXFromPixel(context.singleImagePoint.x())
        : context.routeImagePoint.x() * projectSetInfo.dRadioX;
    const double xMile = roadWidth > 0.0 ? qBound(0.0, rawXMile, roadWidth) : qMax(0.0, rawXMile);
    const QString mileText = QString::fromLocal8Bit("(X:%1,Y:%2)").arg(xMile, 0, 'f', 3).arg(encoderMile, 0, 'f', 3);

	const QString projectRoadStandard = QString::fromLocal8Bit(projectSetInfo.strRoadStandard);
	QString roadStandard = HnProjectEnums::roadTypeEnumToQString(currentPointMile.roadStandard);
	if ((currentPointMile.roadStandard == 0 || roadStandard.isEmpty()) && !projectRoadStandard.isEmpty())
	{
		roadStandard = projectRoadStandard;
	}
	const QString roadGrad = currentPointMile.roadGradStr.isEmpty()
		? QString::fromLocal8Bit(projectSetInfo.strRoadLevel)
		: currentPointMile.roadGradStr;
	const int roadType = currentPointMile.roadType;
	const int drawType = currentPointMile.drawType;
    const QString pixName = QFileInfo(context.imageName).fileName();
    QString streetPixName;
    auto currentProject = hnDataManager::getDataManager()->getCurrentProject();
    if (currentProject)
    {
        const double streetTrueMile = currentProject->enclToTrueMile(currentBottomEncoderMile());
        streetPixName = currentStreetPictureNameForStatus(streetTrueMile);
    }

    return QString::fromLocal8Bit("桩号：%1\t里程:%2\t路面标准：%3\t"
        "路面材质：%4\t路面等级：%5\t病害模式：%6\t拼接图片坐标：%7\t单张图片坐标：%8\t路面图片：%9\t景观图片：%10\t")
        .arg(trueMile, 0, 'f', 3)
        .arg(mileText)
        .arg(roadStandard)
        .arg(roadType == 0 ? QString::fromLocal8Bit("沥青") :
        (roadType == 1 ? QString::fromLocal8Bit("水泥") : QString::fromLocal8Bit("砂石")))
        .arg(roadGrad)
        .arg(drawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式"))
        .arg(QString::number(context.routeImagePoint.x()) + "," + QString::number(context.routeImagePoint.y()))
        .arg(QString::number(context.singleImagePoint.x()) + "," + QString::number(context.singleImagePoint.y()))
        .arg(pixName)
        .arg(streetPixName);
}

hn2dPixWidget::~hn2dPixWidget()
{
	clearLineCameraAreaGuide();
}

void hn2dPixWidget::slotDiseaseChanged()
{
	this->m_currentWidgetDiseases.clear();
	// 数据库病害增删改统一刷新 SDK scene；非 SDK 路径在基类中退化为普通 update()。
	this->refreshSdkDiseaseLayer();
}

bool hn2dPixWidget::diseasePointToWidgetPointAfterBrowse(
	const pixImagePoint& point,
	bool up,
	QPoint& widgetPoint) 
{
	Q_UNUSED(up);

	if (point.pixName.isEmpty())
	{
		return false;
	}

	if (point.pixPoint.x() < 0 || point.pixPoint.y() < 0)
	{
		return false;
	}

	const QSize imageSize = currentPaintImageSize();
	if (!imageSize.isValid() || imageSize.isEmpty())
	{
		return false;
	}

	QPoint bigImagePoint =singleImagePointToBigImagePoint(point.pixPoint, point.pixName);

	if (bigImagePoint.x() < 0 || bigImagePoint.y() < 0)
	{
		return false;
	}

	const double scaleX = this->width() * 1.0 / imageSize.width();
	const double scaleY = this->height() * 1.0 / imageSize.height();

	widgetPoint = QPoint(
		qRound(bigImagePoint.x() * scaleX),
		qRound(bigImagePoint.y() * scaleY)
	);
	/*
	如果你实际显示区域不是整个 widget，而是某个 imageRect，就改成：

widgetPoint = QPoint(
	imageRect.left() + qRound(bigImagePoint.x() * scaleX),
	imageRect.top()  + qRound(bigImagePoint.y() * scaleY)
	*/
	return true;
}

bool hn2dPixWidget::makeLittleFrameHn2dRect(const QRect&bigImageRect, hn2dRectI& hnRect)
{
	QRect rect = bigImageRect.normalized();

	//用中心点确定小框属于哪个图
	QString centerPixName;
	QPoint centerSinglePoint = this->bigImagePointToSingleImagePoint(rect.center(), &centerPixName);

	if (centerPixName.isEmpty()||centerSinglePoint.x() <0|| centerSinglePoint.y()<0)
	{
		qWarning() << "makeLittleFrameHn2dRect invalid center" <<
			"rect=" << rect
			<< "centerPixName=" << centerPixName <<
			"centerSinglePoint = " << centerSinglePoint;
		return false;
	}
	auto mileIter = m_pixNameHnMileMap.constFind(centerPixName);
	if (mileIter == m_pixNameHnMileMap.constEnd())
	{
		qWarning() << "makeLittleFrameHn2dRect mile not found"
			<< "centerPixName = " << centerPixName
			<< "rect = " << rect;
		return false;
	}

	const double dmi = mileIter.value().dEnclMile;

	//当前图片在大图上的左上角位置
	const QPoint imageTopLeft = this->singleImagePointToBigImagePoint(QPoint(0, 0), centerPixName);

	auto toHnPoint = [&](const QPoint& bigPoint)->hn2dPointWithMileI
	{
		int x = bigPoint.x() - imageTopLeft.x();
		int y = bigPoint.y() - imageTopLeft.y();

		x = qBound(0, x, m_pixWidth -1);
		y = qBound(0, y, m_pixHeight-1);
		if (m_isHMirrored)
		{
			x = m_pixWidth - 1 - x;

		}
		if (m_isVMirrored)
		{
			y = m_pixHeight - 1 - y;
		}

		hn2dPointWithMileI p;
		p.x = x;
		p.y = y;
		p.m_dmi = dmi;
		return p;
	};
	hnRect.p0 = toHnPoint(rect.topLeft());
	hnRect.p1 = toHnPoint(rect.topRight());
	hnRect.p2 = toHnPoint(rect.bottomRight());
	hnRect.p3 = toHnPoint(rect.bottomLeft());
	return true;
}

