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
#include<QProgressDialog>
 


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
	this->loadPix(pixNames);

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
	//更新所有病害
	QVector<hnRoadDiseaseInfo> allRoadDiseaes = hnApp::hnDataManager::getDataManager()->getDiseaseService()->getAllDiseases();
   
	QProgressDialog progress(QStringLiteral("检测到旧版本病害，自动进行更新，此过程仅一次，耗时较长，请耐心等待"),
		QString(), 0, allRoadDiseaes.size(), this);
	progress.setWindowTitle(QStringLiteral("处理中..."));
	progress.setWindowModality(Qt::ApplicationModal);
	progress.setMinimumDuration(0);
	progress.setValue(0);
	for (int i = 0 ; i<allRoadDiseaes.size() ;++i)
	{ 
		auto& dis = allRoadDiseaes[i];
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
		progress.setValue(i + 1);
		QApplication::processEvents();

	}
	progress.setValue(allRoadDiseaes.size());
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
	//调整图片
//	QElapsedTimer timer;

	//timer.restart();
	
	this->adjustImage(image);
	//qDebug() << "PERF adjustImage" << timer.elapsed() << "ms";


	//timer.restart(); 
	// 绘制数据库加载内容
	this->drawDatabaseLoadData(image);
	//qDebug() << "PERF drawDatabaseLoadData" << timer.elapsed() << "ms";

	//timer.restart();
	//绘制临时内容
	this->drawTmpData(image);
	//qDebug() << "PERF drawTmpData" << timer.elapsed() << "ms";

	//timer.restart();
	//绘制打标分界线
	this->drawMarkValue(image);
	//qDebug() << "PERF drawMarkValue" << timer.elapsed() << "ms";
	

	//timer.restart();
	//设置当前的hnMile
	this->setCurrentHnMile();
	//qDebug() << "PERF setCurrentHnMile" << timer.elapsed() << "ms";
}

void hn2dPixWidget::mousePressEvent(QMouseEvent * event)
{
	m_lblCoordinates->hide();
	bool isvalid = isValidArea(event);
	if (!isvalid)
	{
		return;
	}
	// 使用右键进行删除自动化模式病害，所以这里右键取消话病害功能不使用
#if 0
	//右键取消画病害
	if (event->button() == Qt::RightButton && this->m_workMode == WorkMode::ADD_MODE)
	{
		this->slot_cancelDrawDiseases();
	}
#endif

	if (event->button()== Qt::RightButton)
	{
		selectDisease(event->pos());
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
				

				m_pendingRightClickDeletePoint = event->pos();
				const int serial = ++m_pendingRightClcikDeleteSerial;
				QTimer::singleShot(QApplication::doubleClickInterval(), this, [this, serial]()
				{
					if (serial !=m_pendingRightClcikDeleteSerial)
					{
						return;
					}
					if (m_pendingRightClickDeletePoint.x()< 0||m_pendingRightClickDeletePoint.y()<0)
					{
						return;
					}

				 
				  
					this->littleFrameRightButtonDragDelete(m_pendingRightClickDeletePoint);
					m_pendingRightClickDeletePoint = QPoint(-1, -1);
				
				});



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

	//2025.11.3 新增最后一个左键点击点与鼠标位置之间的连线（虚线）
	if (this->m_isDrawingDisease && this->addLineDiseType && !m_tmpPaintLineDiseasePoints.empty())
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

	//画临时病害
	if (this->m_isDrawingDisease && !this->addLineDiseType)
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


	//如果正在放大图片，更新
	if (m_isMagnifyPix && !this->m_isDrawingDisease)
	{
		//记录当前鼠标位置
		m_magnifyBigImagePos = this->screenPointToBigImagePoint(event->pos());
		this->update();
	}

	//更新原始比例窗口
	QImage originalImage = this->getOriginalImage(event->pos(), m_tmpPixImageWithoutDisease, m_originalWidgetWidth, m_originalWidgetHeight);
	sig_mousePosImageChanged(originalImage);

	//获取状态栏所需要的信息，并发送信号
	QString statusInfo = generateStatusInfo(event->pos());
	emit this->signal_statusInfoChanged(statusInfo);
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
	m_magnifyBigImagePos = QPoint(-100, -100);
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
	if (event->key() == Qt::Key_D)
	{
		this->littleDrawRectType = !this->littleDrawRectType;
	}

	// 按下 N键 时，结束左键连续点击添加线状病害
	if (event->key() == Qt::Key_N && this->addLineDiseType)
	{

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
	else
	{
		return true;
	}

	return false;
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
	QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
	QPoint bigImageEnd = this->singleImagePointToBigImagePoint(m_diseaseEndPoint.pixPoint, m_diseaseEndPoint.pixName);

	QRect diseaseRect(bigImageStart, bigImageEnd);
	diseaseRect = diseaseRect.normalized();

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


	//弹出添加病害窗口
	addDiseaseDialog dialog(diseaseNameAndKey, false);
	dialog.setWindowTitle(QString::fromLocal8Bit("添加病害"));
	dialog.setDiseaseAttributeEnabled(false);
	QString diseaseTypeName;
	QString diseaseMark;
	if (dialog.exec() == QDialog::Accepted)
	{
		diseaseTypeName = dialog.getDiseaseTypeName();
		diseaseMark = dialog.getDiseaseMarkInfo();
	}
	else
	{
		this->m_isDrawingDisease = false;
		this->m_isAllowDrawPix = false;
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
		

		hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo); 
 
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



		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0); 

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1); 

		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(newDisease); 
		 
		this->update();
		 
		m_seclectedDiseases.clear();
	}
}

void hn2dPixWidget::drawDatabaseLoadData(QImage & image)
{
	QElapsedTimer timer;

	timer.restart();
	if (!this->m_isAllowDrawPix)
		return;  

	auto project = hnDataManager::getDataManager()->getCurrentProject();

	QVector<hnRoadDiseaseInfo> diss;
	 
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->getRoadDiseasesInRange(m_beginEncoderMile, m_endEncoderMile, diss);
	this->m_currentWidgetDiseases = diss.toStdVector();
	if (this->m_frameMode == FrameMode::BIG_FRAME)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image, 0);
	}

	//将加载后的自动化模式病害画到界面上
	if (this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		this->drawLittleFrameDisease(this->m_currentWidgetDiseases, image);
	}
	//qDebug() << "PERF drawLittleFrameDisease" << timer.elapsed() << "ms";
	//绘制设计模式面状病害
	if (FrameMode::DESIGN_FACETS == m_frameMode || FrameMode::DESIGN_LINE == m_frameMode)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image, 2);

		//绘制设计模式线状病害
		this->drawLineDiseases(m_currentWidgetDiseases, image);
	}


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

	//如果要画放大镜内容
	if (m_isMagnifyPix && m_magnifyBigImagePos.x() > 0)
	{
		image = this->drawMagnifyPixRectangle(m_magnifyBigImagePos,
			m_tmpPixImageWithoutDisease, image);
	}
	const qint64 drawLineAndDrawBigImage = timer.elapsed();
	timer.restart();
	if (getDiseases >= 5 || drawMs >= 10 || drawLineAndDrawBigImage >= 10 || !m_currentWidgetDiseases.empty())
	{
		qDebug().noquote() << "[HN_PERF][2DDrawDatabaseLoadData]"
			<< "getDiseasesMs=" << getDiseases
			<< "drawDiseaseMs=" << drawMs
			<< "drawLineMagnifyMs=" << drawLineAndDrawBigImage
			<< "diseaseCount=" << m_currentWidgetDiseases.size()
			<< "frameMode=" << static_cast<int>(m_frameMode)
			<< "beginMile=" << m_beginEncoderMile
			<< "endMile=" << m_endEncoderMile
			<< "imageSize=" << QString("%1x%2").arg(image.width()).arg(image.height());
	}
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
	int drawType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;

	hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease);

 

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

	hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(disease);
	 
	 

	m_isAllowDrawPix = true;
	this->update();
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
	QFileInfo fileInfo(pixName);
	QString pixNameResult;
	int lastSlash = pixName.lastIndexOf('/');
	if (lastSlash!=-1)
	{
		int secondLastSlash = pixName.lastIndexOf('/', lastSlash - 1);
		if (secondLastSlash !=-1)
		{
			pixNameResult = pixName.mid(secondLastSlash + 1);
		}
		else
		{
			pixNameResult = fileInfo.fileName();
		}
	}
	else
	{
		pixNameResult = fileInfo.fileName();
	}
	 

	hnMile currentPointMile = this->getHnMileFromPoint(bigImagePoint);
	statusInfo = QString::fromLocal8Bit("图片底部桩号：%1	图片底部里程：%2	桩号：%3	里程:%4	路面标准：%5	"
		"路面材质：%6	路面等级：%7	病害模式：%8	屏幕坐标：%9	拼接图片坐标：%10	单张图片坐标：%11	图片名称：%12	")
		.arg(currentPointMile.dTrueMile, 0, 'f', 0)
		.arg(currentPointMile.dEnclMile, 0, 'f', 0)
		.arg(caculateTrueMile(bigImagePoint), 0, 'f', 3)
		.arg(calculateEncoderMile(bigImagePoint), 0, 'f', 3)
		.arg(HnProjectEnums::roadTypeEnumToQString(currentPointMile.roadStandard))
		.arg(currentPointMile.roadType == 0 ? QString::fromLocal8Bit("沥青") :
		(currentPointMile.roadType == 1 ? QString::fromLocal8Bit("水泥") : QString::fromLocal8Bit("砂石")))
		.arg(currentPointMile.roadGradStr)
		.arg(currentPointMile.drawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式"))
		.arg(QString::number(eventPos.x()) + "," + QString::number(eventPos.y()))
		.arg(QString::number(bigImagePoint.x()) + "," + QString::number(bigImagePoint.y()))
		.arg(QString::number(singleImagePoint.x()) + "," + QString::number(singleImagePoint.y()))
		.arg(pixNameResult)
		;

	return statusInfo;
}


vector<hn2dRectI> hn2dPixWidget::generateLargeFrameHn2dRectVector(const QRect & rect)
{
	hn2dRectI hnRect;

	hnRect.p0 = getHnPoint2dWithMileI(rect.topLeft());
	hnRect.p1 = getHnPoint2dWithMileI(rect.topRight());
	hnRect.p2 = getHnPoint2dWithMileI(rect.bottomRight());
	hnRect.p3 = getHnPoint2dWithMileI(rect.bottomLeft());

	//人工模式只有一个框
	std::vector<hn2dRectI> vec;
	vec.push_back(hnRect);
	return vec;
}

vector<hn2dRectI> hn2dPixWidget::createLineDisease2dCoordVec(QVector<pixImagePoint> lineDiseasePoints)
{
	vector<hn2dRectI> result;
	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QPoint bigImagePoint = this->singleImagePointToBigImagePoint(lineDiseasePoint.pixPoint, lineDiseasePoint.pixName);
		hn2dRectI hnRect;
		hnRect.p0 = getHnPoint2dWithMileI(bigImagePoint);
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
		hn3dRectI hnRect;
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
		//如果其中有一个点无效的话，则返回空的数组
		std::vector<hn3dRectI> vec;
		return vec;
	}

	//人工模式只有一个框
	std::vector<hn3dRectI> vec;
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

hnCommon::hnRoadDiseaseInfo hn2dPixWidget::
caculateBigFrameDiseaseAttribute(const QRect & diseaseRect, const hnDiseaseSetInfo &diseaseSetInfo, const QString& makinfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnCommon::hnRoadDiseaseInfo disease;

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	disease.dWidth =std::round(  diseaseRect.width()* projectSetInfo.dRadioX *100)/100;		//病害宽度std::round(dis.dReaWidth*100.0) / 100.0;
	disease.dLength = std::round(diseaseRect.height()* projectSetInfo.dRadioY*100)/100;		//病害长度
	disease.nPixelWid = diseaseRect.width();							//病害像素宽度
	disease.nPixelLen = diseaseRect.height();							//病害像素长度
	disease.dArea = disease.dReaWidth * disease.dRealLen;				//病害面积

	//计算2d的坐标信息
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

	//中心里程
	disease.dMileage = this->calculateBigFrameCenterMile(diseaseRect);
	//开始里程
	disease.dDmiStart = this->calculateBigFrameBeginMile(diseaseRect);
	//结束里程
	disease.dDmiEnd = this->calculateBigFrameEndMile(diseaseRect);

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
	hnCommon::hnRoadDiseaseInfo result;

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
	if (this->m_tmpLittleFrameDiseaseRects.isEmpty())
	{
		resetLittleFrameDrawState();
		return false;
	}

	//如果病害无效，则取消画病害
	if (!this->isTmpDiseaseRoadTypeValid(1))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		resetLittleFrameDrawState();
		return false;
	}


	//计算病害种类
	QStringList diseaseTypeList;
	QList<QPair<QString, QString>> diseaseNameAndKey;
	QVector<hnDiseaseSetInfo> diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(this->m_firstHnMile);
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	//弹出添加病害窗口
	addDiseaseDialog dialog(diseaseNameAndKey, false);
	dialog.setWindowTitle(QString::fromLocal8Bit("添加病害"));
	dialog.setDiseaseAttributeEnabled(false);
	QString diseaseTypeName;
	QString diseaseMark;
	if (dialog.exec() == QDialog::Accepted)
	{
		diseaseTypeName = dialog.getDiseaseTypeName();
		diseaseMark = dialog.getDiseaseMarkInfo();
	}
	else
	{
		resetLittleFrameDrawState();

		m_isDrawingDisease = false;
		m_isAllowDrawPix = true;
		m_isAllowLinked = true;
		return false;
	}

	hnRoadDiseaseTable roadDisease;

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
      hnCommon::hnRoadDiseaseInfo diseaseInfo;
	
	//计算病害属性
	 diseaseInfo = this->caculateLittleFrameDiseaseAttribute(
		 this->m_tmpLittleFrameDiseaseRects, selectDiseaseSetInfo,diseaseMark);



	//自动化模式要特殊处理 检查沉陷类病害计算
	if (!diseaseInfo.vec3dRect.empty() && true == m_isOPenDepthCaculate)
	{
		//自动化模式沉陷类病害的判别
		bool isDrawDisease = this->caculateLittleFrameDiseaseDepth(diseaseInfo,
			*this,
			&rectAlgorithm::mergeRects,
			*this,
			&drawDiseases::generateLargeFrameHn3dRectVector,
			m_tmpLittleFrameDiseaseRects);
		if (false == isDrawDisease)
		{
			resetLittleFrameDrawState();

			return false;
		}
	}


	//计算病害id
	diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());

	if (!this->isTmpDiseaseAreaValid(diseaseInfo))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害面积与规范不符，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		resetLittleFrameDrawState();

		return false;
	}

	hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo);
	resetLittleFrameDrawState();

	return true;
}

void hn2dPixWidget::drawLittleFrameDisease(vector<hnRoadDiseaseInfo>& diseases, QImage &image)
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
	//	std::vector<hnRoadDiseaseInfo> currentDis = m_currentWidgetDiseases; 
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

		hnRoadDiseaseInfo disease = srcDisease;

		if (hitIndex >= 0 && hitIndex < disease.vec2dRect.size())
		{
			disease.vec2dRect.erase(disease.vec2dRect.begin() + hitIndex);
		}

		if (!disease.vec3dRect.empty() && hitIndex >= 0 && hitIndex < disease.vec3dRect.size())
		{
			disease.vec3dRect.erase(disease.vec3dRect.begin() + hitIndex);
		}

		disease.dArea = 0.01 * disease.vec2dRect.size();
		disease.nRectCnt = disease.vec2dRect.size();
		disease.n3dCnt = disease.vec3dRect.size();

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

		
	

		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0); 

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1); 
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease); 
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(newDisease);
		 
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

QVector<QRect> hn2dPixWidget::caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo & disease)
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

hnCommon::hnRoadDiseaseInfo hn2dPixWidget::caculateLittleFrameDiseaseAttribute(const QVector<QRect>& diseaseRects, const hnDiseaseSetInfo & diseaseSetInfo ,const QString& MarkInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnCommon::hnRoadDiseaseInfo disease;

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
 
	disease.dArea = 0.1 * 0.1 *diseaseRects.size();		//病害面积
 
	//计算2d的坐标信息
	vector<hn2dRectI> disease2dPointVector = this->generateLittleFrameHn2dRectVector(diseaseRects);
	disease.vec2dRect = disease2dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->generateLittleFrameHn3dRectVector(diseaseRects);
		disease.vec3dRect = disease3dPointVector;
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
	return disease;
}

vector<hn2dRectI> hn2dPixWidget::generateLittleFrameHn2dRectVector(const QVector<QRect>& rects)
{
	std::vector<hn2dRectI> dstVec;
	dstVec.reserve(rects.size());

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
	hn3dRectI hnRect;
	hn2d3dCoordinates tool;

	for (const QRect &rect : qAsConst(rects))
	{
		//用每个自动化模式的中心点来映射，由于格子数量是一致的，只用一个点映射就可以了。
		hn3dPointWithMileI p;
		std::vector<hn3dRectI> tmpVec;

		p = getHnPoint3dWithMileI(rect.center());
		tmpVec = tool.get3dLittleRects(p);
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
	QVector<QRect> dstRects;		//目标的自动化模式rect数组
	QVector<QRect> bigImageRects;	//大图片坐标系中每张图片的自动化模式数组
	QVector<QString> pixNames;		//记录已经添加的图片名字

	for (auto point : qAsConst(pixImagePoints))
	{
		bigImageRects.clear();

		if (!pixNames.contains(point.pixName))
		{
			pixNames.append(point.pixName);
			//将单张图片的自动化模式数组转成大 图片的自动化模式数组
			bigImageRects = this->calculateBigImageRects(point.pixName);
			//添加到目标rects中
			dstRects += bigImageRects;
		}
	}

	return dstRects;
}

QMap<double, QString>::const_iterator hn2dPixWidget::getPreviousStakeIterator(const QMap<double, QString>&map, int curMile)
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
			this->drawLineOnImage(line, 20, Qt::blue, image);
			QPainter painter(&image);
			QPen pen;
			pen.setWidth(m_lineWidth);
			pen.setColor(Qt::red);
			painter.setPen(pen);
			QFont font = painter.font();
			font.setBold(true);
			font.setPixelSize(150);
			painter.setFont(font);
			QString diseaseInfo = markTypeStr + MyCommonMethods::convertMileToString(curMark.dTrueMile) + "_" + typeStr;
			int midWidth = (startPoint.x() + endPoint.x()) / 2;
			if (midWidth>750)
			{
				midWidth = midWidth - 750;
			}
			painter.drawText(QPoint(midWidth, startPoint.y()), diseaseInfo);
		}
	
	}
}

bool hn2dPixWidget::isTmpDiseaseAreaValid(const hnCommon::hnRoadDiseaseInfo& disease)
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



hnCommon::hnRoadDiseaseInfo hn2dPixWidget::getMousePosDisease( const QPoint & mousePoint)
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

hn2dPixWidget::~hn2dPixWidget()
{
	 
}

void hn2dPixWidget::slotDiseaseChanged()
{
	this->m_currentWidgetDiseases.clear();
	this->update();
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

	if (m_tmpPixImageWithoutDisease.isNull())
	{
		return false;
	}

	QPoint bigImagePoint =singleImagePointToBigImagePoint(point.pixPoint, point.pixName);

	if (bigImagePoint.x() < 0 || bigImagePoint.y() < 0)
	{
		return false;
	}

	const double scaleX = this->width() * 1.0 / m_tmpPixImageWithoutDisease.width();
	const double scaleY = this->height() * 1.0 / m_tmpPixImageWithoutDisease.height();

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

