#include "hn3dPixWidget.h"
#include "../hnProject/hnProject.h"
#include "addDiseaseDialog.h"
#include "../hnProject/hn3DProject.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QToolTip>
#include "../hnDiseaseService.h"
using namespace hnApp;
using namespace hnPro;

hn3dPixWidget::hn3dPixWidget(QWidget *parent)
//: hnBrowsePixWidget(parent), m_strGreyImaePath(""), m_strRGBImagePath("")
{
	this->m_workMode = WorkMode::NO_MODE;
	this->m_isDrawingDisease = false;

	m_fontSize = 120;
	m_lineWidth = 10;

	m_widgetType = WIDGET_3D;

	connect(hnApp::hnDataManager::getDataManager()->getDiseaseService(),
		SIGNAL(diseaseChanged()), this, SLOT(slotDiseaseChanged()));
}


void hn3dPixWidget::load3DImagePictures()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	if (!hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		return;
	}
	 
	//工程类型
	m_projectType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType();

	//获取翻转配置
	m_isHMirrored = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored();
	m_isVMirrored = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored();

	//获取高度比例和宽度比例
	this->m_heightScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	this->m_widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();

	//获取图片宽度高度
	this->m_pixWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();
	this->m_pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	// 获取里程信息
	this->m_2dMileVector = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector();

	// 获取灰度影像图像
	this->m_vecGreyImageName = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getAllImage();
	this->m_strGreyImaePath = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getGreyImagePath();

	//灰度图转编码器里程数组
	this->m_3dEncoderMileVector = this->grayImageNamesToEncoderMileVector();
	//编码器里程数组转 里程-hnmile数组
	this->m_encoderMileHnMileMap = this->encoderMilesToEncoderMileHnMileMap();

	// 获取深度影像
	this->m_vecRGBImageName = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getAllImage();
	this->m_strRGBImagePath = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getDepthImagePath();

	if (this->m_vecRGBImageName.size() == 0 || this->m_vecGreyImageName.size() == 0)
	{
		return;
	}

	QString strPicPath;
	QStringList pixNames;
	//灰度图模式
	if (this->m_3dImageMode == ImageShowMode::Gray)
	{
		for (int i = 0; i < m_vecGreyImageName.size(); i++)
		{
			strPicPath = this->m_strGreyImaePath + "/GREY" + this->m_vecGreyImageName[i];
			pixNames.append(strPicPath);
		}
	}
	//深度图模式
	else if (this->m_3dImageMode == ImageShowMode::RGB)
	{
		for (int i = 0; i < this->m_vecRGBImageName.size(); i++)
		{
			strPicPath = this->m_strRGBImagePath + "/RGB" + this->m_vecRGBImageName[i];
			pixNames.append(strPicPath);
		}
	}

	//计算单张图片的自动化模式数组
	this->m_singleImageLittleFrameRects = this->createSingleImageLittleFrameRect();

	//加载图片
	this->loadPix(pixNames);

	//设置底部帧数前后各加载的帧数
	//为当前帧数的两倍
	this->setLoadFrameNum(m_currentWidgetFrameNum * 2);

	//确定框选模式
	this->initFrameMode();

#ifdef  ALL_LITTLE_DRAW
	//20251017改动
	bool isBig = false;
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
	QVector<hnRoadDiseaseInfo> allRoadDiseaes = getAllRoadDisease();
	for (auto& dis : allRoadDiseaes)
	{
		reCalculateDiseaseSizeAndSave(dis, true);
	}
#endif
}

void hn3dPixWidget::setScrollBarValue(const int scrollBarValue)
{
	emit sig_scrollBarValueChanged(scrollBarValue);
}


void hn3dPixWidget::setMileVector(QVector<hnMile>& hnMiles)
{
	this->m_2dMileVector.clear();
	this->m_2dMileVector = hnMiles;
}



QString hn3dPixWidget::generateStatusInfo(const QPoint & eventPos)
{
	QString statusInfo;
	QPoint bigImagePoint = this->screenPointToBigImagePoint(eventPos);
	QString pixName;
	QPoint singleImagePoint = this->screenToSingleImagePoint(eventPos, pixName);
	QFileInfo fileInfo(pixName);
	pixName = fileInfo.fileName();

	double currentPixButtomTrueMile;
	double currentPixButtomEncoderMile;
	hnMile currentPointMile = this->getHnMileFromPoint(bigImagePoint);
	if (hnCommon::PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		currentPixButtomTrueMile = currentPointMile.dTrueMile;
		currentPixButtomEncoderMile = currentPointMile.dEnclMile;
	}
	else
	{
		QString pixNameWithoutPath = this->pixName3dConvertWithoutPath(pixName);
		const double buttomMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixNameWithoutPath);
		currentPixButtomEncoderMile = buttomMile;
		currentPixButtomTrueMile = this->encoderMileToTrueMile(currentPixButtomEncoderMile);
	}


	statusInfo = QString::fromLocal8Bit("图片底部桩号：%1\t图片底部里程：%2\t桩号：%3\t里程:%4\t路面标准：%5\t"
		"路面材质：%6\t路面等级：%7\t病害模式：%8\t屏幕坐标：%9\t拼接图片坐标：%10\t单张图片坐标：%11\t图片名称：%12\t")
		.arg(currentPixButtomTrueMile, 0, 'f', 0)
		.arg(currentPixButtomEncoderMile, 0, 'f', 0)
		.arg(this->caculateTrueMile(bigImagePoint), 0, 'f', 3)
		.arg(this->calculateEncoderMile(bigImagePoint), 0, 'f', 3)
		.arg(HnProjectEnums::roadTypeEnumToQString(currentPointMile.roadStandard))
		.arg(currentPointMile.roadType == 0 ? QString::fromLocal8Bit("沥青") :
		(currentPointMile.roadType == 1 ? QString::fromLocal8Bit("水泥") : QString::fromLocal8Bit("砂石")))
		.arg(currentPointMile.roadGradStr)
		.arg(currentPointMile.drawType == 0 ? QString::fromLocal8Bit("人工模式") : QString::fromLocal8Bit("自动化模式"))
		.arg(QString::number(eventPos.x()) + "," + QString::number(eventPos.y()))
		.arg(QString::number(bigImagePoint.x()) + "," + QString::number(bigImagePoint.y()))
		.arg(QString::number(singleImagePoint.x()) + "," + QString::number(singleImagePoint.y()))
		.arg(pixName)
		;

	return statusInfo;
}

void hn3dPixWidget::drawSomeThingOnImage(QImage & image)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject() || !hnDataManager::getDataManager()->getCurrentProject())
	{
		return;
	}

	if (true == this->m_vecGreyImageName.isEmpty() || true == this->m_vecRGBImageName.isEmpty())
	{
		return;
	}

	//调节图像参数
	this->adjustImage(image);

	//绘制数据库加载数据
	this->drawDatabaseLoadData(image);

	//绘制临时数据
	this->drawTmpData(image);

	//设置当前的hnMile
	this->setCurrentHnMile();
}


void hn3dPixWidget::mousePressEvent(QMouseEvent * event)
{ 

	if (!isValidArea(event))
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

	if (event->button() == Qt::RightButton)
	{
		selectDisease(event->pos());
	}

	if (m_frameMode == hnFrameMode::BIG_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:
				this->bigFrameAddDisease(event->pos());
				break;
			case hnWorkMode::DELETE_MODE:
				//删除人工模式病害
				this->commonDeleteDisease(event->pos(), hnFrameMode::BIG_FRAME);
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
	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE: 
			{
				if (this->addLineDiseType)   //左键连续点击模式添加线状病害
				{
					this->m_isDrawingDisease = true;
					//如果正在画临时病害，就不允许画图片
					this->m_isAllowDrawPix = false;

					//允许画最后点击点与鼠标位置连线（虚线）
					this->m_isAllowDrawDashLine = true;

					//修改是否联动
					this->m_isAllowLinked = false;
					if (!this->m_isEndAddPoint)
					{
						m_diseaseAddPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseAddPoint.pixName);

						//记录单张图片的点到折线数组中
						m_littleSingleImagePoints.append(m_diseaseAddPoint);
						//转化单张图片的点数组到拼接图片的点数组
						QPoint bigImagePoint = singleImagePointToBigImagePoint(m_diseaseAddPoint.pixPoint, m_diseaseAddPoint.pixName);
						m_litteBigImagePoints.append(bigImagePoint);

						m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;
						m_tmpPaintLineDiseasePoints.append(m_diseaseAddPoint);
						m_tmpLastPaintLineDiseasePoints.append(m_diseaseAddPoint);
						this->update();
					}
				}
				else   //自动跟踪鼠标移动轨迹模式添加病害
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
						m_diseaseStartPoint.pixPoint = QPoint(-1, -1);
						m_diseaseEndPoint.pixPoint = QPoint(-1, -1);
						m_tmpLittleFrameDiseaseRects.clear();
						m_littleSingleImagePoints.clear();
						m_litteBigImagePoints.clear();
					}

					//记录开始点
					m_diseaseStartPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseStartPoint.pixName);
					//记录结束点
					m_diseaseEndPoint = m_diseaseStartPoint;

					//cwb 20240909注释  这部分移到了上方判断病害有效区间处 
					/*QPoint bigImagePoint = this->screenPointToBigImagePoint(event->pos());

					this->m_firstHnMile = this->getHnMileFromPoint(bigImagePoint);*/
				}
			}
				break;
			case hnWorkMode::DELETE_MODE:
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
					this->m_isAllowDrawDashLine = false;

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
			}
			else
			{
				// 修改标志，记录按下的位置
				this->m_isRightDeleteMouseDown = true;
				m_RightDeleteMousePoint = event->pos();
				// 右键按下不拖拽时也删除自动化模式
				this->littleFrameRightButtonDragDelete(event->pos());
			}
		}
	}
	if (m_frameMode == hnFrameMode::DESIGN_FACETS)
	{

		if (event->button() == Qt::LeftButton)
		{
			switch (m_workMode)
			{
			case hnWorkMode::NO_MODE:
				break;
			case hnWorkMode::ADD_MODE:
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
		if (event->button() == Qt::RightButton)
		{

		}
	} 
	if (m_frameMode == DESIGN_LINE)
{
	if (event->button() == Qt::LeftButton)
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
} 

	//添加控制点
	if (WorkMode::ADD_CTRL_POINT == m_workMode && event->button() == Qt::LeftButton)
	{
		this->addCtrlPoint(event->pos());
	}

	//删除控制点
	if (WorkMode::DELETE_MODE == m_workMode && event->button() == Qt::LeftButton)
	{
		this->deleteCtrlPoint(event->pos());
	}

	// 编辑控制点(查看控制点信息)
	if (WorkMode::EDIT_MODE == m_workMode && event->button() == Qt::LeftButton)
	{
		this->editCtrlPoint(event->pos());
	} 
	bool isDesignMode = FrameMode::DESIGN_FACETS == m_frameMode || FrameMode::DESIGN_LINE == m_frameMode;

	// 设计模式 删除
	if (WorkMode::DELETE_MODE == m_workMode && Qt::LeftButton == event->button() && isDesignMode)
	{
		this->commonDeleteDisease(event->pos(), FrameMode::DESIGN_FACETS);
		this->commonDeleteDisease(event->pos(), hnFrameMode::DESIGN_LINE);
	}

	// 设计模式 编辑
	if (Qt::MouseButton::LeftButton == event->button() && WorkMode::EDIT_MODE == m_workMode && isDesignMode)
	{
		this->bigFrameEditProcess(event->pos());
		this->commonEditDisease(event->pos(), FrameMode::DESIGN_LINE);
	}

	// 设计模式 合并
	if (Qt::MouseButton::LeftButton == event->button() && WorkMode::MERGE == m_workMode && isDesignMode)
	{
		this->bigFrameMergeDiseases(event->pos());
		this->mergeLineDisease(event->pos());
	}

	

	// 获取点击点的编码器里程
	if (event->button() == Qt::LeftButton && m_workMode == WorkMode::GET_MILE)
	{
		//获取点击点的编码器里程
		m_encoderMile = this->caculateEncoderMileByScreenPoint(event->pos());
		//转为大imagePoint 保存下来
		m_seclectPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_seclectPoint.pixName);
	}
	hn2d3dPixBaseWidget::mousePressEvent(event);
}

void hn3dPixWidget::mouseReleaseEvent(QMouseEvent * event)
{

	// 鼠标右键释放
	if (event->button() == Qt::MouseButton::RightButton)
	{
		this->m_isRightDeleteMouseDown = false;
		m_RightDeleteMousePoint = QPoint(-1, -1);			// 右键释放，将坐标设置为无效值
	}

}



void hn3dPixWidget::mouseMoveEvent(QMouseEvent * event)
{
	//currentMousePos = event->pos();
	currentMousePos = event->screenPos().toPoint();				// 记录鼠标在屏幕的位置

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	if (m_pixNameMap.empty())
	{
		return;
	}

	//2025.11.3 新增最后一个左键点击点与鼠标位置之间的连线（虚线）
	if (this->m_isDrawingDisease && this->addLineDiseType  && this->m_isAllowDrawDashLine && !m_tmpPaintLineDiseasePoints.empty())
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
		

		//记录结束点到线状病害绘制数组中
		m_tmpPaintLineDiseasePoints = m_tmpLineDiseasePoints;

		//三维视图画点越界（相对于二维视图）的处理
		if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
		{
			int x = m_diseaseEndPoint.pixPoint.x();
			this->autoCorrectXIn3dView(x);
			m_diseaseEndPoint.pixPoint.setX(x);
		}
		m_tmpLastPaintLineDiseasePoints.append(m_diseaseEndPoint);
		m_tmpPaintLineDiseasePoints.append(m_diseaseEndPoint);

		if (!isSuspended)
		{
			this->setCursor(Qt::CrossCursor);
			//转化单张图片的点数组到拼接图片的点数组
			//记录单张图片的点到折线数组中
			m_littleSingleImagePoints.append(m_diseaseEndPoint);
			m_litteBigImagePoints.clear();
			for (auto point : qAsConst(m_littleSingleImagePoints))
			{
				QPoint bigImagePoint = singleImagePointToBigImagePoint(point.pixPoint, point.pixName);
				m_litteBigImagePoints.append(bigImagePoint);
			}
		}
		else
		{
			if (m_litteBigImagePoints.size()<=0)
			{
				return;
			}
			pixImagePoint  lastPointWithImage = m_littleSingleImagePoints.last();
			//在最后一个点和当前位置之间绘制虚线
			QPoint lastPoint = singleImagePointToBigImagePoint(lastPointWithImage.pixPoint, lastPointWithImage.pixName);
			pixImagePoint currentEndPoint;
			currentEndPoint.pixName = m_diseaseEndPoint.pixName;
			currentEndPoint.pixPoint = this->screenToSingleImagePoint(event->pos(), m_diseaseEndPoint.pixName);
			QPoint currentBigImagePoint = singleImagePointToBigImagePoint(currentEndPoint.pixPoint, currentEndPoint.pixName);
			double distance = QLineF(lastPoint, currentBigImagePoint).length();

			lastPoint_Suspending = lastPoint;
			current_Suspending = currentBigImagePoint;
			m_litteBigImagePoints.clear();
			for (auto point : qAsConst(m_littleSingleImagePoints))
			{
				QPoint bigImagePoint = singleImagePointToBigImagePoint(point.pixPoint, point.pixName);
				m_litteBigImagePoints.append(bigImagePoint);
			}
			this->setCursor(Qt::OpenHandCursor);
			if (distance <= 50)
			{
				isSuspended = false;

			}
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
		this->littleFrameRightButtonDragDelete(event->pos());
	}

#endif


	//如果正在放大图片，更新
	if (m_isMagnifyPix)
	{
		//记录当前鼠标位置
		m_magnifyBigImagePos = this->screenPointToBigImagePoint(event->pos());
		this->update();
	}

	//更新原始比例窗口
	QImage originalImage = this->getOriginalImage(event->pos(), m_tmpPixImageWithoutDisease, m_originalWidgetWidth, m_originalWidgetHeight);
	sig_mousePosImageChanged(originalImage);

	//更新状态栏信息
	QString statusInfo = this->generateStatusInfo(event->pos());
	emit signal_statusInfoChanged(statusInfo);
}

void hn3dPixWidget::wheelEvent(QWheelEvent * event)
{
	//在滚轮滚动的时候，允许画病害图片
	this->m_isAllowDrawPix = true;

	//延时发送事件，避免出问题
	QTimer::singleShot(5, [this, event]() {
		//触发鼠标移动事件
		QMouseEvent *mouseEvent = new QMouseEvent(QEvent::MouseMove, this->mapFromGlobal(QCursor().pos()),
			Qt::NoButton, Qt::NoButton, Qt::NoModifier);
		QApplication::sendEvent(this, mouseEvent);

		delete mouseEvent;
	});

	bool up = event->delta() > 0 ? true : false;
	if (this->m_workMode == WorkMode::ADD_MODE&&
		this->m_isDrawingDisease
		&& this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		//小框的绘制临时停止绘制策略
		isSuspended = true;

	}
	moveMouse(up, false);

	//QTimer::singleShot(20, [this, event]() {
	//	if (this->addLineDiseType && this->m_isDrawingDisease && !this->m_isEndAddPoint)
	//	{
	//		this->m_isAllowDrawPix = false;
	//		this->update();
	//	}
	//});
}

void hn3dPixWidget::leaveEvent(QEvent * event)
{
	m_magnifyBigImagePos = QPoint(-100, -100);
}

void hn3dPixWidget::keyPressEvent(QKeyEvent * event)
{

	if (event->key() == Qt::Key_Delete)
	{
		// qDebug() << QStringLiteral("按下了删除键");
		QPoint screenPoint = QCursor::pos();
		QPoint widgetPoint = this->mapFromGlobal(screenPoint);
		if (m_frameMode == LITTLE_FRAME)
		{
			// 按下 删除键 时，获取鼠标位置，删除病害
			this->littleFrameRightButtonDragDelete(widgetPoint);
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
		this->m_isAllowDrawDashLine = false;

		//生成多段折线病害自动化模式
		this->m_currentLittleFrameRects = this->createLittleFrameRects(m_littleSingleImagePoints);
		if (m_littleSingleImagePoints.size() > 1)
		{
			for (int i = 1; i < m_littleSingleImagePoints.size(); ++i)
			{
				QPoint startPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i - 1].pixPoint, m_littleSingleImagePoints[i - 1].pixName);
				QPoint endPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i].pixPoint, m_littleSingleImagePoints[i].pixName);
				QLine line(startPoint, endPoint);

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
}


QVector<QPoint> hn3dPixWidget::createBrokenLinePoints(hnRoadDiseaseInfo & disease)
{
	QVector<QPoint> result;
	for (auto rect : qAsConst(disease.vec3dRect))
	{
		result.push_back(hn3dPointToBigImageQtPoint(rect.p0));
	}

	return result;
}

void hn3dPixWidget::drawDatabaseLoadData(QImage & image)
{
	if (!m_isAllowDrawPix)
	{
		return;
	}
	QVector<hnRoadDiseaseInfo> diss;
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->getRoadDiseasesInRange(m_beginEncoderMile, m_endEncoderMile, diss);
	this->m_currentWidgetDiseases = diss.toStdVector();

	auto projectInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	QString standard = HnProjectEnums::roadTypeEnumToQString(hnApp::hnDataManager::getDataManager()->getCurrentProject()->getBaseStandard());
	//加载控制点
	m_currentCtrlPoints.clear();
	QString stateMent = QString("SELECT * FROM %1 WHERE Mileage >= %2 AND Mileage <= %3")
		.arg(CTRL_POINT_TABLE).arg(m_beginEncoderMile).arg(m_endEncoderMile);
	hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->readData(m_currentCtrlPoints, stateMent.toLocal8Bit().data());

	if (PROJECT_23D_TYPE == m_projectType)
	{
		if (false == m_seclectPoint.pixName.isEmpty())
		{
			QPoint startPoint = this->singleImagePointToBigImagePoint(QPoint(0, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QPoint endPoint = this->singleImagePointToBigImagePoint(QPoint(m_pixWidth, m_seclectPoint.pixPoint.y()), m_seclectPoint.pixName);
			QLine line(startPoint, endPoint);
			this->drawLineOnImage(line, 10, Qt::yellow, image);
		}
	}

	//将加载后的人工模式病害画到界面上
	if (this->m_frameMode == FrameMode::BIG_FRAME)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image);
	}

	//将加载后的自动化模式病害画到界面上
	if (this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		this->drawLittleFrameDisease(this->m_currentWidgetDiseases, image);
	}

	//绘制设计模式面状病害
	if (FrameMode::DESIGN_FACETS == m_frameMode || FrameMode::DESIGN_LINE == m_frameMode)
	{
		this->drawBigFrameDisease(this->m_currentWidgetDiseases, image);

		//绘制设计模式线状病害
		this->drawLineDiseases(m_currentWidgetDiseases, image);
	}

	//将加载后的控制点绘制到image上
	this->drawCtrlPoint(image, m_currentCtrlPoints);

	//记录临时内容画板
	m_tmpContectImage = image;

	//如果要画放大镜内容
	if (m_isMagnifyPix && m_magnifyBigImagePos.x() > 0)
	{
		image = this->drawMagnifyPixRectangle(m_magnifyBigImagePos,
			m_tmpPixImageWithoutDisease, image);
	}
}

void hn3dPixWidget::drawTmpData(QImage & image)
{

	if (!m_isAllowDrawPix)
	{
		image = this->m_tmpContectImage;
		//画新增临时自动化模式矩形病害
		if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::LITTLE_FRAME)
		{

			if (this->littleDrawRectType)
			{
				QRect bigRect = this->drawTmpLittleBigFrameDisease(image);

				//创造自动化模式鼠标移动轨迹的自动化模式
				this->m_currentLittleFrameRects = this->createLittleFrameRects(m_littleSingleImagePoints);

				//判断鼠标移动轨迹折线与当前自动化模式矩形数组 相交的矩形数组
				hn2d3dCoordinates tool;
				this->m_tmpLittleFrameDiseaseRects = tool.crossRectOver(bigRect,  this->m_currentLittleFrameRects);

				//画出自动化模式
				const QColor rectColor = Qt::red;
				this->drawRectsOnImage(image, QVector<QRect>::fromList(this->m_tmpLittleFrameDiseaseRects.toList()), 10, rectColor, Qt::SolidLine);
			}
			else
			{
				//创造自动化模式鼠标移动轨迹经过的自动化模式数组
				m_currentLittleFrameRects = this->createLittleFrameRects(m_littleSingleImagePoints);

				 

				if (this->addLineDiseType)
				{
					if (m_littleSingleImagePoints.size() > 1)
					{
						for (int i = 1; i < m_littleSingleImagePoints.size(); ++i)
						{
							QPoint startPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i - 1].pixPoint, m_littleSingleImagePoints[i - 1].pixName);
							QPoint endPoint = singleImagePointToBigImagePoint(m_littleSingleImagePoints[i].pixPoint, m_littleSingleImagePoints[i].pixName);
							QLine line(startPoint, endPoint);
							QPainter painter(&image);
							QPen pen;
							pen.setColor(Qt::yellow);
							pen.setWidth(10);
							pen.setStyle(Qt::DashLine);
							painter.setPen(pen);
							painter.drawLine(line);

							//判断鼠标移动轨迹折线与当前自动化模式矩形数组 相交的矩形数组
							hn2d3dCoordinates tool;
							QVector<QRect> tmpDiseaseRects = tool.crossLineOver(line, this->m_currentLittleFrameRects);

							//画出自动化模式
							const QColor rectColor = Qt::red;
							this->drawRectsOnImage(image, QVector<QRect>::fromList(tmpDiseaseRects.toList()), 10, rectColor, Qt::SolidLine);
						}
					}

					//2025.11.3 新增最后一个鼠标左键点击点与鼠标位置连线（虚线）
					if (m_tempPoints.size() == 2 && m_isAllowDrawDashLine)
					{
						this->drawTempDashLine(image);

					}
				}
				else
				{

					if (isSuspended)
					{
						//	double distance = QLineF(lastPoint_Suspending, current_Suspending).length();

						QLine line(lastPoint_Suspending, current_Suspending);
						QPainter painter(&image);
						QPen pen;
						pen.setColor(Qt::yellow);
						pen.setWidth(10);
						pen.setStyle(Qt::DashLine);
						painter.setPen(pen);
						painter.drawLine(line);
					}

					//计算当前视图所有自动化模式与鼠标移动轨迹相交的矩形框
					hn2d3dCoordinates tool;
					this->m_tmpLittleFrameDiseaseRects = tool.crossOver(m_litteBigImagePoints, this->m_currentLittleFrameRects);

					//画出自动化模式
					const QColor rectColor = Qt::red;
					this->drawRectsOnImage(image, QVector<QRect>::fromList(this->m_tmpLittleFrameDiseaseRects.toList()), 12, rectColor, Qt::SolidLine);

				}

			}
		
		}

		//画新增临时人工模式矩形病害
		if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::BIG_FRAME)
		{
			this->drawTmpLargeFrameDisease(image);
		}
		//绘制新增临时面状病害
		if (this->m_isDrawingDisease && this->m_frameMode == FrameMode::DESIGN_FACETS)
		{
			this->drawTmpLargeFrameDisease(image);
		}

		//绘制临时设计模式线状病害
		if (this->m_isDrawingDisease &&  FrameMode::DESIGN_LINE == this->m_frameMode)
		{
			this->drawTmpLineDiseases(image);
		}
	}
}

bool hn3dPixWidget::isTmpDiseaseRoadTypeValid(const int & frameType)
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
	//找到所有的hnMile
	for (auto pixName : qAsConst(pixNames))
	{
		hnMile mile = getHnMileBy3dPixName(pixName);
		hnMiles.append(mile);
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

void hn3dPixWidget::drawBigFrameDisease(const vector<hnRoadDiseaseInfo>& diseases, QImage & image)
{
	QVector<QRect> rects;
	for (auto disease : diseases)
	{
		if (disease.vec3dRect.empty() || disease.nDrawType == 1)
		{
			continue;
		}

		hn3dRectI hnRect = disease.vec3dRect.at(0);

		QRect diseaseRect = hn3dRectToBigImageQtRect(hnRect);

		hnImagePainter imagePainter;
		const int fontSize = m_fontSize;
		imagePainter.setFontPixelSize(70);
		imagePainter.setBoardWidth(12);  
		int mile = qRound(hnApp::hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(disease.dMileage)); //cwb 20240522修改
	//	int mile = qRound(this->encoderMileToTrueMile( disease.dMileage));

		int qian = mile / 1000;
		int bai = mile - qian * 1000;
		QString mileStr = QStringLiteral("桩号：") + "K" + QString::number(qian) + "+" + QString::number(bai).rightJustified(3, '0');

		QString diseaseInfo = QString::fromLocal8Bit(disease.strDisName) + "_" + QString::number(disease.nID) + "\n" 
			+ mileStr + "\n" 
		    + QStringLiteral("计算长度：") + QString::number(disease.dRealLen, 'f', 2) + "\n" 
			+ QStringLiteral("计算宽度：") + QString::number(disease.dReaWidth, 'f', 2)+"\n"
			+ QStringLiteral("计算面积：") + QString::number(disease.dArea, 'f', 2);
		QString mark = QString::fromLocal8Bit(disease.strRemark);
		if (!mark.isEmpty())
		{
			diseaseInfo += "\n" + QStringLiteral("病害备注：") + mark;
		}
		QColor rectColor(144, 238, 144);
		//QColor rectColor = Qt::red; 
		QColor textColor = Qt::blue;
		if (selectedDiseaseId == disease.nID)
		{
			rectColor = Qt::yellow;
		}
		if (this->isSeclectedMergeDisease(disease) )
		{
			rectColor = Qt::yellow;
		}

		imagePainter.drawRectOnImage(image, diseaseRect, diseaseInfo, Qt::red, Qt::yellow);
	}

}

void hn3dPixWidget::bigFrameAddDisease(const QPoint & mousePoint)
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


void hn3dPixWidget::bigFrameEditProcess(const QPoint & mousePoint)
{
	auto disease =  getMousePosDisease(mousePoint);
	if (!disease.isValid())
	{
		return;
	} 
	hn3dRectI  hnRect = disease.vec3dRect.at(0); 
	QRect diseaseRect = this->hn3dRectToBigImageQtRect(hnRect);
	m_seclectedDiseases.clear();
	m_seclectedDiseases.append(disease);
	//按照人工模式左上角的点来计算hnMile
	this->editDisease(disease, diseaseRect.topLeft());
	m_seclectedDiseases.clear();
	
}

void hn3dPixWidget::bigFrameMergeDiseases(const QPoint & screenPoint)
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
		hn3dRectI  hnRect1 = m_seclectedDiseases.at(0).vec3dRect.at(0);
		QRect diseaseRect1 = hn3dRectToBigImageQtRect(hnRect1);
		//第二个病害的大image QRect
		hn3dRectI  hnRect2 = m_seclectedDiseases.at(1).vec3dRect.at(0);
		QRect diseaseRect2 = hn3dRectToBigImageQtRect(hnRect2);
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

		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->generateLargeFrameHn3dRectVector(newDiseaseRect);
		newDisease.vec3dRect = disease3dPointVector;

		//清空2d坐标信息
		newDisease.vec2dRect.clear();

		if (false == newDisease.vec3dRect.empty() && true == m_isOPenDepthCaculate)
		{
			//计算人工模式病害深度信息
			if (false == this->caculateBigFrameDiseaseDepth(newDisease))
			{
				m_seclectedDiseases.clear();
				return;
			}
		}

		if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
		{
			//计算2d的坐标信息
			vector<hn2dRectI> disease2dPointVector = this->generateLargeFrameHn2dRectVector(newDiseaseRect);

			//如果数组是空的，则映射的2维病害有问题，提示用户
			if (disease2dPointVector.size() == 0)
			{
				QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
					QString::fromLocal8Bit("确定"));
			}
			else
			{

				newDisease.vec2dRect = disease2dPointVector;
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



		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease); 

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);

		if (hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease( newDisease)); 
		this->update();

		//清空选中的病害数组
		m_seclectedDiseases.clear();
	}
}

void hn3dPixWidget::editDisease(hnRoadDiseaseInfo &disease, const QPoint & mousePoint)
{
	hnMile mile = this->getHnMileFromPoint(mousePoint);

	//弹出添加病害窗口 让用户选择病害类型
	QStringList diseaseTypeList;
	QVector<hnDiseaseSetInfo> diseaseSetInfos;
	if (PROJECT_23D_TYPE == m_projectType)
	{
		diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
	}
	else
	{
		auto setting = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
			(ROAD_WORK_TYPE)setting.nDrawType, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
	} 
	QList<QPair<QString, QString>> diseaseNameAndKey;
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append(qMakePair(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	//弹出添加病害窗口  
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
		return;
	}

	hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease( disease);

	 

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
	//变形类病害深度计算
	if (false == disease.vec3dRect.empty() && true == m_isOPenDepthCaculate && nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		//计算人工模式病害深度信息
		if (0 == disease.nDrawType)
		{
			if (!this->caculateBigFrameDiseaseDepth(disease))
			{
				return;
			}
		}
		else
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

	//计算病害面积信息
	hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);

	 
	auto mark8Bit = diseaseMark.toLocal8Bit();
	auto markStd = mark8Bit.toStdString();
	strcpy(disease.strRemark, markStd.c_str());
	//写入数据库
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease( disease);

	 
}

void hn3dPixWidget::updateLittleFrameDisease(hnRoadDiseaseInfo & disease)
{
	// 如果自动化模式数量为0 ，则直接删除整个病害
	if (disease.vec3dRect.size() <= 0|| disease.vec2dRect.size() <= 0)
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
			if (!this->m_isRightDeleteMouseDown)
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

	}
	auto setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

	hnApp::hnDataManager::getDataManager()->getDiseaseService()->updateDisease(disease);
	 

	m_isAllowDrawPix = true;
	this->update();

}


QRect hn3dPixWidget::hn3dRectToBigImageQtRect(const hn3dRectI & hnRect)
{
	QRect qrect;

	qrect.setTopLeft(this->hn3dPointToBigImageQtPoint(hnRect.p0));
	qrect.setTopRight(this->hn3dPointToBigImageQtPoint(hnRect.p1));
	qrect.setBottomRight(this->hn3dPointToBigImageQtPoint(hnRect.p2));
	qrect.setBottomLeft(this->hn3dPointToBigImageQtPoint(hnRect.p3));

	return qrect;
}

QPoint hn3dPixWidget::hn3dPointToBigImageQtPoint(const hn3dPointWithMileI & hn3dPoint)
{
	QPoint qpoint;

	//里程转图片名称
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return QPoint();
	}

	//获取图片的绝对路径
	auto iter = this->m_encoderMileGrayPixNameMap.find(hn3dPoint.bottomEncoderMile);
	if (m_encoderMileGrayPixNameMap.end() == iter)
	{
		return qpoint;
	}
	QString pixName = this->m_encoderMileGrayPixNameMap.find(hn3dPoint.bottomEncoderMile).value();

	if (this->m_3dImageMode == hn3dImageMode::Gray)
	{
		pixName = this->m_strGreyImaePath + "/GREY" + pixName;
	}
	else
	{
		pixName = this->m_strRGBImagePath + "/RGB" + pixName;
	}

	//镜像的x，镜像的y
	int mirroredX = hn3dPoint.x;
	int mirroredY = hn3dPoint.y;
	if (m_isHMirrored)
	{
		mirroredX = m_pixWidth - hn3dPoint.x;
	}
	if (m_isVMirrored)
	{
		mirroredY = m_pixHeight - hn3dPoint.y;
	}

	//把单张图片的坐标转为整个labelImage的坐标
	qpoint = this->singleImagePointToBigImagePoint(QPoint(mirroredX, mirroredY), pixName);

	return qpoint;
}

QPoint hn3dPixWidget::ctrlPointToBigImagePoint(const hnKZDDataInfo & ctrlPoint)
{
	QPoint qpoint;

	//里程转图片名称
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return QPoint();
	}

	//获取图片的绝对路径
	QString pixName = QString::fromLocal8Bit(ctrlPoint.strImageName);

	if (this->m_3dImageMode == hn3dImageMode::Gray)
	{
		pixName = this->m_strGreyImaePath + "/GREY" + pixName;
	}
	else
	{
		pixName = this->m_strRGBImagePath + "/RGB" + pixName;
	}

	//镜像的x，镜像的y
	int mirroredX = ctrlPoint.nLocX;
	int mirroredY = ctrlPoint.nLocY;
	if (m_isHMirrored)
	{
		mirroredX = m_pixWidth - mirroredX;
	}
	if (m_isVMirrored)
	{
		mirroredY = m_pixHeight - mirroredY;
	}

	//把单张图片的坐标转为整个labelImage的坐标
	qpoint = this->singleImagePointToBigImagePoint(QPoint(mirroredX, mirroredY), pixName);

	return qpoint;
}

vector<hn3dRectI> hn3dPixWidget::generateLargeFrameHn3dRectVector(const QRect & rect)
{
	hn3dRectI hnRect;

	hnRect.p0 = getHnPoint3dWithMileI(rect.topLeft());
	hnRect.p1 = getHnPoint3dWithMileI(rect.topRight());
	hnRect.p2 = getHnPoint3dWithMileI(rect.bottomRight());
	hnRect.p3 = getHnPoint3dWithMileI(rect.bottomLeft());

	//人工模式只有一个框
	std::vector<hn3dRectI> vec;
	vec.push_back(hnRect);
	return vec;
}

hnCommon::hn3dPointWithMileI hn3dPixWidget::getHnPoint3dWithMileI(const QPoint & point)
{
	QString picName;
	QPoint singleImagePoint;
	singleImagePoint = this->bigImagePointToSingleImagePoint(point, &picName);

	hnCommon::hn3dPointWithMileI point3dWithMileI;
	//赋值x
	point3dWithMileI.x = singleImagePoint.x();

	//赋值y
	point3dWithMileI.y = singleImagePoint.y();

	//对镜像做处理
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsHMirrored())
	{
		point3dWithMileI.x = m_pixWidth - singleImagePoint.x();
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getIsVMirrored())
	{
		point3dWithMileI.y = m_pixHeight - singleImagePoint.y();
	}

	//赋值mile
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return hn3dPointWithMileI();
	}
	//picName为绝对路径，转成图片名字
	QFileInfo fileInfo(picName);
	QString pixFileName = fileInfo.fileName();
	point3dWithMileI.bottomEncoderMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixFileName);


	return point3dWithMileI;
}

hnMile hn3dPixWidget::getHnMileFromPoint(const QPoint & allImagePoint)
{
	QString picName;
	

	////获取二三维的编码器里程差值  
	//double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();

	////转换成像素
	//double yScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;

	//int pixelDiff = encoderMileDiff / yScale;

	////获取新的点 
	//QPoint new2dBigImagePoint = QPoint(allImagePoint.x(), allImagePoint.y() + pixelDiff);	//像素和里程方向是反的，所以，这里的y是加的

	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(allImagePoint, &picName);

	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return hnMile();
	}
	//获取图片的不含路径的名字
	QFileInfo fileInfo(picName);
	picName = fileInfo.fileName();

	double mile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(picName);

	hnMile dstHnMile;
	auto iter = this->m_encoderMileHnMileMap.find(mile);
	if (iter == this->m_encoderMileHnMileMap.end())
	{
		return dstHnMile;
	}
	dstHnMile = iter.value();
	// double dmi =  this->trueMileToEncoderMile(dstHnMile.dTrueMile);

	//获取二三维的编码器里程差值  
	//dstHnMile.dTrueMile = this->encoderMileToTrueMile(dstHnMile.dEnclMile);
	return dstHnMile;
}

QRect hn3dPixWidget::bigFrameDiseaseAutoWidth(const QRect & bigImageRect)
{
	if (PROJECT_23D_TYPE != m_projectType)
	{
		return bigImageRect;
	}
	QRect dstRect = bigImageRect;

	const double roadWidth2d = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	const double roadWidth3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	const double scale3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	const int pixel2d3dBoarderWidth = 0.5*((roadWidth3d - roadWidth2d) / scale3d);
	const int leftBoarder = pixel2d3dBoarderWidth;
	const int rightBoarder = m_pixWidth - pixel2d3dBoarderWidth;

	if (dstRect.left() < leftBoarder)
	{
		dstRect.setLeft(leftBoarder);
	}
	if (dstRect.right() > rightBoarder)
	{
		dstRect.setRight(rightBoarder);
	}

	return dstRect;
}

vector<hn3dRectI> hn3dPixWidget::createLineDisease3dCoord(const QVector<pixImagePoint>& lineDiseasePoints)
{
	vector<hn3dRectI> result;

	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QPoint bigImage = this->pixImagePointToBigImagePoint(lineDiseasePoint);
		hn3dRectI hnRect;
		hnRect.p0 = this->getHnPoint3dWithMileI(bigImage);
		result.push_back(hnRect);
	}

	return result;
}

vector<hn2dRectI> hn3dPixWidget::createLineDisease2dCoord(const QVector<pixImagePoint>& lineDiseasePoints)
{
	vector<hn2dRectI> result;

	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QPoint bigImage = this->pixImagePointToBigImagePoint(lineDiseasePoint);
		hn2dRectI hnRect;
		hnRect.p0 = this->getHnPoint2dWithMileI(bigImage);
		if (hnRect.p0.m_dmi < 0)
		{
			continue;
		}
		result.push_back(hnRect);
	}

	return result;

}

double hn3dPixWidget::calculateEncoderMile(const QPoint & bigImagePoint)
{
	double encoderMile = 0.0f;
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return encoderMile;
	}

	QString pixName;
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);

	if (hnCommon::PROJECT_23D_TYPE == m_projectType)
	{
		hnMile pointHnMile = getHnMileFromPoint(bigImagePoint);
		encoderMile = (this->m_pixHeight - singleImagePoint.y()) * this->m_heightScale + pointHnMile.dEnclMile;
	}
	else
	{
		pixName = this->pixName3dConvertWithoutPath(pixName);
		const double pixMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixName);

		encoderMile = (this->m_pixHeight - singleImagePoint.y()) * this->m_heightScale + pixMile;
	}

	return encoderMile;
}

double hn3dPixWidget::caculateTrueMile(const QPoint & bigImagePoint)
{
	double encoderMile = this->calculateEncoderMile(bigImagePoint);

	if (hnCommon::PROJECT_23D_TYPE == m_projectType)
	{
		const double diff2d3d = hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
		encoderMile += diff2d3d;
	}

	double trueMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(encoderMile);

	return trueMile;
}

double hn3dPixWidget::calculateBigFrameCenterMile(const QRect & rect)
{
	double centerMile = 0.0f;

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return centerMile;
	}

	centerMile = (this->caculateEncoderMileByBigImagePoint(rect.topLeft()) + this->caculateEncoderMileByBigImagePoint(rect.bottomLeft())) / 2;

	return centerMile;
}

double hn3dPixWidget::calculateBigFrameBeginMile(const QRect & rect)
{
	double beginMile = 0.0f;

	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return beginMile;
	}

	beginMile = this->caculateEncoderMileByBigImagePoint(rect.bottomLeft());

	return beginMile;
}

double hn3dPixWidget::calculateBigFrameEndMile(const QRect & rect)
{
	double endMile = 0.0f;
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return endMile;
	}

	endMile = this->caculateEncoderMileByBigImagePoint(rect.topLeft());
	return endMile;
}

void hn3dPixWidget::setCurrentHnMile()
{
	auto pixNameIter = this->m_pixNameMap.find((int)m_buttomFrameIdx);

	if (pixNameIter == this->m_pixNameMap.end())
	{
		return;
	}

	QString pixName = pixNameIter.value();

	//根据图片名称找到编码器里程
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	pixName = this->pixName3dConvertWithoutPath(pixName);

	double encoderMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixName);
	if (PROJECT_23D_TYPE == m_projectType)
	{
		////从map表中查找hnMile
		//hnMile resultHnMile = this->m_encoderMileHnMileMap.find(encoderMile).value();

		//hnApp::hnDataManager::getDataManager()->getCurrentProject()->setCurrentRoadMile(resultHnMile);
	}
	else
	{
		hnDataManager::getDataManager()->getCurrentProject()->setCurrent3dRoadDmi(encoderMile);
	}

}

std::vector<hnMile> hn3dPixWidget::getCurrentWidgetHnMiles()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return std::vector<hnMile>();
	}

	std::vector<hnMile> resultHnMiles;

	//前后各多算五张 为了修正二三维中间hnMile的差值
	const int extraFrameNum =10;
	int extraBeginFrame = m_buttomFrameIdx - extraFrameNum;
	int extraEndFrame = m_buttomFrameIdx + extraFrameNum;

	//前后各多算一页
	//int extraBeginFrame = m_buttomFrameIdx - m_currentWidgetFrameNum;
	if (extraBeginFrame < 1)
	{
		extraBeginFrame = 1;
	}
	//int extraEndFrame = m_buttomFrameIdx + 2 * m_currentWidgetFrameNum;
	if (extraEndFrame > this->m_pixNameMap.size())
	{
		extraEndFrame = this->m_pixNameMap.size();
	}
	QString pixName;
	double encoderMile = 0.0f;
	for (auto idx = extraBeginFrame; idx <= extraEndFrame; idx++)
	{
		//根据帧序号，获取图片名字
		pixName = this->m_pixNameMap.find(idx).value();
		hnMile hnmile = this->getHnMileBy3dPixName(pixName);
		if (hnmile.picturePath.isEmpty())
		{
			continue;
		}

		resultHnMiles.push_back(this->getHnMileBy3dPixName(pixName));
	}

	return resultHnMiles;
}

bool hn3dPixWidget::isInDisease(const QPoint & screenPoint, const hnRoadDiseaseInfo & disease, const FrameMode & frameMode)
{
	QPoint imagePoint = this->screenPointToBigImagePoint(screenPoint);

	if (hnFrameMode::BIG_FRAME == frameMode || hnFrameMode::DESIGN_FACETS == frameMode)
	{
		if (disease.vec3dRect.empty())
		{
			return false;
		}
		hn3dRectI  hnRect = disease.vec3dRect.at(0);

		QRect diseaseRect = hn3dRectToBigImageQtRect(hnRect);

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
		//获取病害的三维数组
		std::vector<hn3dRectI>  hn3dRects = disease.vec3dRect;

		if (hn3dRects.empty())
		{
			return false;
		}

		QVector<QRect> diseaseRects;

		for (hn3dRectI rect3d : qAsConst(hn3dRects))
		{
			QRect rect = this->hn3dRectToBigImageQtRect(rect3d);
			diseaseRects.push_back(rect);
		}
		for (QRect rect : qAsConst(diseaseRects))
		{
			if (rect.contains(imagePoint))
			{
				return true;
			}
		}
		return false;
	}
	//线状病害判断鼠标点是否在病害上
	else if (hnFrameMode::DESIGN_LINE == frameMode && 3 == disease.nDrawType)
	{
		return this->isNearbyLineDisease(imagePoint, disease);
	}
	return false;
}

double hn3dPixWidget::caculateEncoderMileByScreenPoint(const QPoint & screenPoint)
{
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return 0.0f;
	}

	QPoint bigImagePoint = this->screenPointToBigImagePoint(screenPoint);

	QString pixName;
	QPoint singlePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	int singleY = singlePoint.y();

	//获取图片底部的里程
	QFileInfo fileInfo(pixName);
	pixName = fileInfo.fileName();
	const double pixBottomMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixName);

	const double encoderMile = pixBottomMile + (m_pixHeight - singleY) * m_heightScale;

	return encoderMile;
}

double hn3dPixWidget::caculateEncoderMile(const pixImagePoint & point)
{
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject())
	{
		return 0.0;
	}
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		return 0.0;
	}

	// 获取底部里程
	QString pixName = this->pixName3dConvertWithoutPath(point.pixName);
	const double pixBottomMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixName);
	//计算里程
	const double mile = pixBottomMile + (m_pixHeight - point.pixPoint.y()) * m_heightScale;
	return mile;
}

double hn3dPixWidget::caculateEncoderMileByBigImagePoint(const QPoint & bigImagePoint)
{
	QString pixName;
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &pixName);
	pixImagePoint point;
	point.pixName = pixName;
	point.pixPoint = singleImagePoint;

	const double mile = this->caculateEncoderMile(point);
	return mile;
}

double hn3dPixWidget::encoderMileToTrueMile(double encoderMile)
{
	const double diff2d3d = hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
	encoderMile += diff2d3d;

	double trueMile = hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(encoderMile);

	return trueMile;
}


double hn3dPixWidget::trueMileToEncoderMile(double trueMile)
{
	double encoderMile = hnDataManager::getDataManager()->getCurrentProject()->trueMileToEncl(trueMile);

	const double diff2d3d = hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
	encoderMile -= diff2d3d;

	return encoderMile;
}



vector<hn2dRectI> hn3dPixWidget::generateLargeFrameHn2dRectVector(const QRect & rect)
{
	hn2dRectI hnRect;

	hnRect.p0 = getHnPoint2dWithMileI(rect.topLeft());
	hnRect.p1 = getHnPoint2dWithMileI(rect.topRight());
	hnRect.p2 = getHnPoint2dWithMileI(rect.bottomRight());
	hnRect.p3 = getHnPoint2dWithMileI(rect.bottomLeft());
	//如果四个点中有一个点无效，则返回空的数组
	if (hnRect.p0.m_dmi == -1 ||
		hnRect.p1.m_dmi == -1 ||
		hnRect.p2.m_dmi == -1 ||
		hnRect.p3.m_dmi == -1)
	{
		std::vector<hn2dRectI> vec;
		return vec;
	}

	//人工模式只有一个框
	std::vector<hn2dRectI> vec;
	vec.push_back(hnRect);
	return vec;
}

hnCommon::hn2dPointWithMileI hn3dPixWidget::getHnPoint2dWithMileI(const QPoint & point)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnCommon::hn2dPointWithMileI();
	}

	//三维和二维图片上有差值，算一下差值
	QPoint bigImagePoint = point;

	//获取二三维的编码器里程差值  
	double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();

	//转换成像素差值
	double yScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	int pixelDiff = encoderMileDiff / yScale;

	//里程和像素的方向是反的，所以这里是-
	int newY = bigImagePoint.y() - pixelDiff;

	bigImagePoint.setY(newY);

	hnCommon::hn2dPointWithMileI dstPoint;
	dstPoint.m_dmi = -1;

	//在四分之一三维图片中的点
	QPoint quartile3dPoint;

	//三维图片的四分之一高
	int quartileHeight = this->m_pixHeight * 0.25;

	//转换坐标  转成某一张三维图片内的坐标
	QString imageName;
	QPoint singleImagePoint = this->bigImagePointToSingleImagePoint(bigImagePoint, &imageName);

	//如果图片名字是空的，点无效，返回dstPoint
	if (imageName.isEmpty())
	{
		return dstPoint;
	}

	//计算二维的x
	//二维的x没变
	quartile3dPoint.setX(singleImagePoint.x());

	//根据二维三维图片的比例关系进行计算
	hn2d3dCoordinates coordinates;
	dstPoint.x = coordinates.single3dXToSingle2dX(quartile3dPoint.x());

	//计算二维的y
	//二维的y是三维点的y除以 四分之一三维的高 的余数
	quartile3dPoint.setY(singleImagePoint.y() % quartileHeight);
	//根据二维三维图片的比例关系进行计算
	int image2dHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;
	dstPoint.y = quartile3dPoint.y() * (image2dHeight *1.0 / quartileHeight);

	//计算二维的里程 
	//二维的里程 = 三维底部的里程  +    2 *(3 - （三维点的y / 四分之一三维的高）)
	QFileInfo fileinfo(imageName);
	QString fileName = fileinfo.fileName();
	double bottomMile = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(fileName);

	dstPoint.m_dmi = bottomMile + 2 * (3 - (singleImagePoint.y() / quartileHeight));

	//对二维的坐标做镜像处理 
	int image2dWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;
	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsHMirrored())
	{
		dstPoint.x = image2dWidth - dstPoint.x;
	}
	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getIsVMirrored())
	{
		dstPoint.y = image2dHeight - dstPoint.y;
	}

	return dstPoint;
}



bool hn3dPixWidget::drawBigFrameProcess()
{
	QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
	QPoint bigImageEnd = this->singleImagePointToBigImagePoint(m_diseaseEndPoint.pixPoint, m_diseaseEndPoint.pixName);

	QRect diseaseRect(bigImageStart, bigImageEnd);
	diseaseRect = diseaseRect.normalized();

	//边界自适应
	if (PROJECT_23D_TYPE == m_projectType)
	{
		diseaseRect = this->bigFrameDiseaseAutoWidth(diseaseRect);
	}


	//如果病害无效，则取消画病害
	if (!this->isTmpDiseaseRoadTypeValid(0))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		return false;
	}

	//获取第一个点的hnMile

	hnMile mile = this->getHnMileFromPoint(bigImageStart);


	hnProjectSetInfo setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	//获取当前hnMile的病害种类
	QStringList diseaseTypeList;
	QVector<hnDiseaseSetInfo> diseaseSetInfos;


	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE)
	{
		if (mile.drawType >1000)
		{
			diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
				(ROAD_WORK_TYPE)0, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
		}
		else
		{
			diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);

		}

	}
	else
	{
		diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
			(ROAD_WORK_TYPE)0, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
	}
	QList<QPair<QString, QString>> diseaseNameAndKey;
	 
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
		this->m_isDrawingDisease = false;
		this->m_isAllowDrawPix = false;
		return false;
	}

	//计算病害属性
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

	diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->getDiseaseTable()->getMaxID(diseaseTableName.toLocal8Bit().data());

	if (!this->isTmpDiseaseAreaValid(diseaseInfo))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害面积与规范不符，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		return false;
	}

	//深度计算
	if (false == diseaseInfo.vec3dRect.empty() && true == m_isOPenDepthCaculate)
	{
		if (false == this->caculateBigFrameDiseaseDepth(diseaseInfo))
		{
			return false;
		}
	}

	//写入数据库
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo);
	  
	return true;
}

void hn3dPixWidget::drawTmpLargeFrameDisease(QImage & image)
{
	//把单张图片的坐标转成大张图片的坐标
	QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
	QPoint bigImageEnd = this->singleImagePointToBigImagePoint(m_diseaseEndPoint.pixPoint, m_diseaseEndPoint.pixName);


	QRect rect(bigImageStart, bigImageEnd);
	rect = rect.normalized();

	hnImagePainter imagePainter;
	imagePainter.setBoardWidth(12);
	imagePainter.setFontPixelSize(120);
	imagePainter.drawRectOnImage(image, rect, QString::fromLocal8Bit(""), Qt::blue, Qt::black);
}

hnCommon::hnRoadDiseaseInfo hn3dPixWidget::caculateBigFrameDiseaseAttribute(const QRect & diseaseRect, const hnDiseaseSetInfo & diseaseSetInfo, const QString& diseaseInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnCommon::hnRoadDiseaseInfo disease;

	disease.dWidth = diseaseRect.width()*this->m_widthScale;		//病害宽度
	disease.dLength = diseaseRect.height()* this->m_heightScale;	//病害长度

	disease.nPixelWid = diseaseRect.width();							//病害像素宽度
	disease.nPixelLen = diseaseRect.height();							//病害像素长度
	disease.dArea = disease.dReaWidth * disease.dRealLen;				//病害面积

	//三维数组生成
	vector<hn3dRectI> disease3dPointVector = this->generateLargeFrameHn3dRectVector(diseaseRect);
	disease.vec3dRect = disease3dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		//二维数组生成
		vector<hn2dRectI> disease2dPointVector = this->generateLargeFrameHn2dRectVector(diseaseRect);

		//如果数组是空的，则映射的2维病害有问题，提示用户
		if (disease2dPointVector.size() == 0)
		{
			QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
				QString::fromLocal8Bit("确定"));
		}
		else
		{
			disease.vec2dRect = disease2dPointVector;
		}
	}
	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	//二三维
	if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
		hnMile firstHnMile = this->m_firstHnMile;
		disease.dRoadWidth = firstHnMile.roadWidth;
		if (disease.vec2dRect.size() > 0)
		{
			disease.dDmi = disease.vec2dRect[0].p0.m_dmi;			//里程

		}
		else
		{
			disease.dDmi = firstHnMile.dEnclMile + encoderMileDiff;			//里程

		}
		disease.nDrawType = m_drawType;
		disease.nLevel = diseaseSetInfo.nLevel;
	
		disease.diseaseWeight = diseaseSetInfo.fWidget;		//权重

		disease.nRSurfaceType = firstHnMile.roadType;
		if (firstHnMile.roadType>1000)
		{
			disease.nRSurfaceType = diseaseSetInfo.nRoadSurfaceType;
			strcpy(disease.strRoadStandard, projectSetInfo.strRoadStandard);
		}
		else
		{
			strcpy(disease.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data()); 
		} 
		//中心里程
		disease.dMileage = this->calculateBigFrameCenterMile(diseaseRect) + encoderMileDiff;
		//开始里程
		disease.dDmiStart = this->calculateBigFrameBeginMile(diseaseRect) + encoderMileDiff;
		//结束里程
		disease.dDmiEnd = this->calculateBigFrameEndMile(diseaseRect) + encoderMileDiff;
	}
	//  单三维
	else
	{
		

		disease.dRoadWidth = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();

		const QString pixNameNoPath = this->pixName3dConvertWithoutPath(m_diseaseStartPoint.pixName);
		const double dmi = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixNameNoPath);
		disease.dDmi = dmi;			//里程
		disease.nDrawType = m_drawType;
		disease.nLevel = diseaseSetInfo.nLevel;
		disease.nRSurfaceType = diseaseSetInfo.nRoadSurfaceType;
		disease.diseaseWeight = diseaseSetInfo.fWidget;		//权重
		strcpy(disease.strRoadStandard, projectSetInfo.strRoadStandard);

		//中心里程
		disease.dMileage = this->calculateBigFrameCenterMile(diseaseRect);
		//开始里程
		disease.dDmiStart = this->calculateBigFrameBeginMile(diseaseRect);
		//结束里程
		disease.dDmiEnd = this->calculateBigFrameEndMile(diseaseRect);
	}


	strcpy(disease.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(disease.strDisName, diseaseSetInfo.strDiseaseTypeName);
	auto mark0 = diseaseInfo.toLocal8Bit();
	auto mark1 = mark0.toStdString();
	strcpy(disease.strRemark, mark1.c_str());

	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	return disease;
}

hnRoadDiseaseInfo hn3dPixWidget::caculateLineDiseaseInfo(QVector<pixImagePoint> lineDiseasePoints, hnDiseaseSetInfo diseaseSetInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnCommon::hnRoadDiseaseInfo result;

	//线病害的长度
	double lenth = this->caculateLineDiseaseLenth(lineDiseasePoints, hn2d3dPixBaseWidget::WIDGET_3D);
	result.dLength = lenth;

	//三维数组生成
	vector<hn3dRectI> disease3dPointVector = this->createLineDisease3dCoord(lineDiseasePoints);
	result.vec3dRect = disease3dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		//二维数组生成
		vector<hn2dRectI> disease2dPointVector = this->createLineDisease2dCoord(lineDiseasePoints);

		//如果数组是空的，则映射的2维病害有问题，提示用户
		if (disease2dPointVector.size() != disease3dPointVector.size())
		{
			QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
				QString::fromLocal8Bit("确定"));
		}
		else
		{
			result.vec2dRect = disease2dPointVector;
		}
	}

	result.nDrawType = m_drawType;
	result.nLevel = diseaseSetInfo.nLevel;
	result.diseaseWeight = diseaseSetInfo.fWidget;		//权重
	//二三维
	if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
		hnMile firstHnMile = this->m_firstHnMile;
		result.dRoadWidth = firstHnMile.roadWidth;
		if (result.vec2dRect.size() > 0)
		{
			result.dDmi = result.vec2dRect[0].p0.m_dmi;			//里程

		}
		else
		{
			result.dDmi = firstHnMile.dEnclMile + encoderMileDiff;			//里程

		}
		result.nRSurfaceType = firstHnMile.roadType;
		strcpy(result.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data());

		//中心里程
		result.dMileage = this->calculateLineDiseaseCenterMile(lineDiseasePoints) + encoderMileDiff;
		//开始里程
		result.dDmiStart = this->calculateLineDiseaseBeginMile(lineDiseasePoints) + encoderMileDiff;
		//结束里程
		result.dDmiEnd = this->calculateLineDiseaseEndMile(lineDiseasePoints) + encoderMileDiff;
	}
	//  单三维
	else
	{
		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

		result.dRoadWidth = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
		const QString pixNameNoPath = this->pixName3dConvertWithoutPath(m_diseaseStartPoint.pixName);
		const double dmi = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixNameNoPath);
		result.dDmi = dmi;			//里程
		result.nRSurfaceType = diseaseSetInfo.nRoadSurfaceType;
		strcpy(result.strRoadStandard, projectSetInfo.strRoadStandard);

		//中心里程
		result.dMileage = this->calculateLineDiseaseCenterMile(lineDiseasePoints);
		//开始里程
		result.dDmiStart = this->calculateLineDiseaseBeginMile(lineDiseasePoints);
		//结束里程
		result.dDmiEnd = this->calculateLineDiseaseEndMile(lineDiseasePoints);
	}



	strcpy(result.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(result.strDisName, diseaseSetInfo.strDiseaseTypeName);

	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(result);
	return result;
}



hnMile hn3dPixWidget::getHnMileBy3dPixName(const QString & pix3dName)
{
	hnMile resultMile;
	QFileInfo info(pix3dName);
	QString pixFileName = info.fileName();

	//根据图片名字 获取里程
	double encoderMile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixFileName);
	//根据里程 获取hnMile
	if (m_encoderMileHnMileMap.find(encoderMile) == m_encoderMileHnMileMap.end())
	{
		return resultMile;
	}
	resultMile = m_encoderMileHnMileMap.find(encoderMile).value();
	return resultMile;
}

QMap < double, hnMile> hn3dPixWidget::encoderMilesToEncoderMileHnMileMap()
{
	 
	QMap<double, hnMile> dstMap; 
	QVector<double> sortedData3d = m_3dEncoderMileVector;
	QVector<hnMile> sortedMile = m_2dMileVector;
	  
	//双指针遍历

	int i = 0;
	int j = 0;
	const int data3dSize = sortedData3d.size();
	const int mileSize = sortedMile.size();

	while (i<data3dSize && j < mileSize)
	{

		const double target = sortedData3d[i];
		const double currentMile = sortedMile[j].dEnclMile;

		if (qFuzzyCompare(target,currentMile))
		{
			dstMap.insert(target, sortedMile[j]);
			i++;
			j++;
		}
		else if (target<currentMile)
		{
			i++;
		}
		else
		{
			j++;
		}
	}

	/*for (auto mile : m_3dEncoderMileVector)
	{
		for (auto srcHnMile : m_2dMileVector)
		{
			if (mile == srcHnMile.dEnclMile)
			{
				dstMap.insert(mile, srcHnMile);
			}
		}
	}*/
	return dstMap;
}

QVector<double> hn3dPixWidget::grayImageNamesToEncoderMileVector()
{
	if (!hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<double>();
	}
	this->m_encoderMileGrayPixNameMap.clear();
	QVector<double> miles;
	double mile;
	for (auto grayPicName : this->m_vecGreyImageName)
	{
		mile = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(grayPicName);

		miles.push_back(mile);
		m_encoderMileGrayPixNameMap.insert(mile, grayPicName);
	}

	return miles;
}

bool hn3dPixWidget::littleFrameProcess()
{
	if (this->m_tmpLittleFrameDiseaseRects.isEmpty())
	{
		return false;
	}

	//如果病害无效，则取消画病害
	if (!this->isTmpDiseaseRoadTypeValid(1))
	{
		QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
			QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
			QString::fromLocal8Bit("确定"));
		return false;
	}


	//计算病害种类

	QVector<hnDiseaseSetInfo> diseaseSetInfos;

	if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(this->m_firstHnMile);
	}
	else
	{
		auto setting = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		diseaseSetInfos = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
			(ROAD_WORK_TYPE)setting.nDrawType, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
	}


	QStringList diseaseTypeList;
	QList<QPair<QString, QString>> diseaseNameAndKey;

	 

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
	QVector<QRect>rects = QVector<QRect>::fromList(this->m_tmpLittleFrameDiseaseRects.toList());
	//计算病害属性
	hnCommon::hnRoadDiseaseInfo diseaseInfo; 
	 diseaseInfo = this->caculateLittleFrameDiseaseAttribute(
	QVector<QRect>::fromList(this->m_tmpLittleFrameDiseaseRects.toList()), selectDiseaseSetInfo, diseaseMark); 

	// 自动化模式 检查沉陷类病害计算
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
		return false;
	}
	hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(diseaseInfo); 
 

	return true;
}


void hn3dPixWidget::littleFrameEditDisease(const QPoint & mousePoint)
{

	 auto disease = getMousePosDisease(mousePoint);
	 if (!disease.isValid())
	 {
		 return;
	 }
	 m_seclectedDiseases.clear();
	 m_seclectedDiseases.append(disease);
	 this->editDisease(disease, mousePoint);
	 m_seclectedDiseases.clear();
}

void hn3dPixWidget::littleFrameRightButtonDragDelete(const QPoint & mousePoint)
{
	//获取鼠标在大图像上的坐标
	QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);
	if (m_currentWidgetDiseases.size() <= 0)
	{
		return;
	}
	std::vector<hnRoadDiseaseInfo> currentDis = m_currentWidgetDiseases;
	//获取鼠标位置的病害
	for (auto disease : currentDis)
	{
		if (disease.vec2dRect.empty() || disease.vec3dRect.empty())
		{
			continue;
		}
		//获取病害的三维数组
		std::vector<hn3dRectI>  hn3dRects = disease.vec3dRect;
		QVector<QRect> diseaseRects;

		for (hn3dRectI rect3d : qAsConst(hn3dRects))
		{
			QRect rect = this->hn3dRectToBigImageQtRect(rect3d);
			diseaseRects.push_back(rect);
		}
		bool project2dOpened = hnDataManager::getDataManager()->getCurrentProject()->get2DProject();
		vector<hn2dRectI> newVec2dRectI;
		vector<hn3dRectI> newVec3dRectI;

		//遍历病害数组，如果包含鼠标点击的点，就编辑病害
		for (int i = 0; i < diseaseRects.size(); i++)
		{
			// 将鼠标位置之外的自动化模式保存
			if (!diseaseRects[i].contains(bigImagePoint))
			{
				newVec3dRectI.push_back(disease.vec3dRect[i]);
				if (project2dOpened)
				{
					newVec2dRectI.push_back(disease.vec2dRect[i]);
				}

			}
		}
		if (newVec3dRectI.size() == disease.vec3dRect.size())
		{
			// 如果自动化模式数量没有发生变化，则不做处理
			continue;
		}

		disease.dArea = 0.01*newVec3dRectI.size();
		disease.vec3dRect = newVec3dRectI;
		disease.nRectCnt = newVec2dRectI.size();
		if (project2dOpened)
		{
			disease.vec2dRect = newVec2dRectI;
		}
		disease.n3dCnt = newVec3dRectI.size();

		// 重新计算病害参数，写入数据库
		this->updateLittleFrameDisease(disease);
	}

}


void hn3dPixWidget::littleFrameMergeDiseases(const QPoint & screenPoint)
{
	//获取选中的病害，添加到数组中
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(screenPoint, disease, hnFrameMode::LITTLE_FRAME))
		{
			if (m_seclectedDiseases.isEmpty())
			{
				m_seclectedDiseases.append(disease);
				update();
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
					update();
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

		//获取两个病害的矩形方格
		auto rects1 = caculateLittleFrameBigImageRects(m_seclectedDiseases.at(0));
		auto rects2 = caculateLittleFrameBigImageRects(m_seclectedDiseases.at(1));

		//合并两个病害的方格
		auto newDiseaseRects = this->mergeRects(rects1, rects2, acrossRects);

		//新病害
		auto newDisease = m_seclectedDiseases.at(0);

		//病害面积
		newDisease.dArea = 0.1 * 0.1 * newDiseaseRects.size();

		//计算3d的坐标信息
		vector<hn3dRectI> disease3dPointVector = this->generateLittleFrameHn3dRectVector(newDiseaseRects);
		newDisease.vec3dRect = disease3dPointVector;

		//清空2d的坐标信息
		newDisease.vec2dRect.clear();

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
				m_seclectedDiseases.clear();
				return;
			}
		}

		//如果打开了2d工程，才进行映射
		if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
		{
			//计算2d的坐标信息
			vector<hn2dRectI> disease2dPointVector = this->generateLittleFrameHn2dRectVector(newDiseaseRects);

			//如果映射后的2d坐标数组无效，则提示用户
			if (disease2dPointVector.size() == 0)
			{
				QMessageBox::warning(this, QString::fromLocal8Bit("警告"), QString::fromLocal8Bit("映射后的坐标无效，映射失败"),
					QString::fromLocal8Bit("确定"));
			}
			else
			{

				newDisease.vec2dRect = disease2dPointVector;
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
		auto secondDisease = m_seclectedDiseases.at(1);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(firstDisease);
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(secondDisease);
	 
		//新病害写入数据库
		hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease( newDisease);
 

		this->update();

		//清空选中的病害数组
		m_seclectedDiseases.clear();
	}
}

QVector<QRect> hn3dPixWidget::caculateLittleFrameRects(QVector<hnRoadDiseaseInfo> diseases)
{
	QVector<QRect> resultRects;

	const int diseasesCorrectSize = 2;
	if (diseasesCorrectSize != diseases.size())
	{
		return resultRects;
	}

	QVector<QRect> rects;

	for (auto disease : diseases)
	{
		//计算自动化模式病害的格子
		rects += caculateLittleFrameBigImageRects(disease);
	}

	QMap<int, QString> pixNames;
	for (auto rect : qAsConst(rects))
	{
		QString imageName;
		QPoint p = rect.center();
		this->bigImagePointToSingleImagePoint(p, &imageName);
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
		auto imageRects = this->calculateBigImageRects(pixName);
		resultRects += imageRects;
	}

	return resultRects;
}

void hn3dPixWidget::drawLittleFrameDisease(const vector<hnRoadDiseaseInfo>& diseases, QImage & image)
{
	for (auto disease : diseases)
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
		QRect diseaseRect;
		if (disease.vec3dRect.size()<=0)
		{
			continue;
		}
		for (const hn3dRectI &hnRect : qAsConst(disease.vec3dRect))
		{
			diseaseRect = this->hn3dRectToBigImageQtRect(hnRect);
			rects.push_back(diseaseRect);
		} 

		hnImagePainter imagePainter;
		const int fontSize = m_fontSize;
		imagePainter.setFontPixelSize(70);

		int mile = qRound(hnApp::hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(disease.dMileage));

		int qian = mile / 1000;
		int bai = mile - qian * 1000;
		QString mileStr = QStringLiteral("桩号：") + "K" + QString::number(qian) + "+" + QString::number(bai).rightJustified(3, '0');
		QString deepStr = disease.dDepth == 0 ? "" : "\n" + QStringLiteral("深度：") + QString::number(disease.dDepth);
		QString diseaseInfo = QString::fromLocal8Bit(disease.strDisName) +"_" + QString::number(disease.nID) +
			+ "\n" + mileStr 
			+ "\n" + QStringLiteral("长度：") + QString::number(disease.dLength)
			+ "\n" + QStringLiteral("宽度：") + QString::number(disease.dWidth)
			+ "\n" + QStringLiteral("面积：") + QString::number(disease.dArea) 
			+ deepStr;
	
		QColor rectColor(Qt::red);
		Qt::PenStyle pen(Qt::SolidLine);
		if (selectedDiseaseId == disease.nID)
		{
			pen = Qt::DashDotDotLine;
		}
		else	if (isSeclectedMergeDisease(disease))
		{
			rectColor = Qt::yellow;
		}
		else
		{
			rectColor = Qt::red;
		}

		//画自动化模式
		this->drawRectsOnImage(image, rects, 10, rectColor, pen);
		//画文字
		QPoint textPos = this->findMinPoint(rects);
		imagePainter.drawTextOnImage(image, diseaseInfo, Qt::yellow, textPos);
	}
}

QVector<QRect> hn3dPixWidget::caculateLittleFrameBigImageRects(const hnRoadDiseaseInfo & disease)
{
	QVector<QRect> rects;

	if (disease.vec3dRect.empty() || 0 == disease.nDrawType)
	{
		return rects;
	}

	QRect diseaseRect;
	for (const hn3dRectI &hnRect : qAsConst(disease.vec3dRect))
	{
		diseaseRect = this->hn3dRectToBigImageQtRect(hnRect);
		rects.push_back(diseaseRect);
	}

	return rects;
}

hnCommon::hnRoadDiseaseInfo hn3dPixWidget::caculateLittleFrameDiseaseAttribute(const QVector<QRect>& diseaseRects, const hnDiseaseSetInfo & diseaseSetInfo, const QString& MarkInfo)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return hnRoadDiseaseInfo();
	}
	hnCommon::hnRoadDiseaseInfo disease;

	hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	//病害面积
	disease.dArea = 0.1 * 0.1 * this->m_tmpLittleFrameDiseaseRects.size();

	//计算3d的坐标信息
	vector<hn3dRectI> disease3dPointVector = this->generateLittleFrameHn3dRectVector(diseaseRects);
	disease.vec3dRect = disease3dPointVector;

	if (hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
	{
		//计算2d的坐标信息
		vector<hn2dRectI> disease2dPointVector = this->generateLittleFrameHn2dRectVector(diseaseRects);
		disease.vec2dRect = disease2dPointVector;
	}
	if (PROJECT_TYPE::PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	{
		double encoderMileDiff = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2d3dMileDiff();
		hnMile firstHnMile = this->m_firstHnMile;
		disease.dRoadWidth = firstHnMile.roadWidth;
		if (disease.vec2dRect.size() > 0)
		{
			disease.dDmi = disease.vec2dRect[0].p0.m_dmi;			//里程

		}
		else
		{
			disease.dDmi = firstHnMile.dEnclMile + encoderMileDiff;			//里程

		}
		disease.nDrawType = m_drawType;
		disease.nLevel = diseaseSetInfo.nLevel;
		disease.diseaseWeight = diseaseSetInfo.fWidget;		//权重
		disease.nRSurfaceType = firstHnMile.roadType;

		auto mark8Bit = MarkInfo.toLocal8Bit();
		auto markStd = mark8Bit.toStdString();
		strcpy(disease.strRemark, markStd.c_str());
		strcpy(disease.strRoadStandard, HnProjectEnums::roadTypeEnumToQString(firstHnMile.roadStandard).toLocal8Bit().data());
		//计算病害中心里程
		disease.dMileage = this->caculateLittleFrameMiddleMile(this->m_tmpLittleFrameDiseaseRects) + encoderMileDiff;
		//开始里程
		disease.dDmiStart = this->calculateLittleFrameBeginMile(m_tmpLittleFrameDiseaseRects) + encoderMileDiff;
		//结束里程
		disease.dDmiEnd = this->calculateLittleFrameEndMile(m_tmpLittleFrameDiseaseRects) + encoderMileDiff;
	}
	else
	{
		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		disease.dRoadWidth = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
		const QString pixNameNoPath = this->pixName3dConvertWithoutPath(m_diseaseStartPoint.pixName);
		const double dmi = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getMileByImage(pixNameNoPath);
		disease.dDmi = dmi;			//里程
		disease.nDrawType = m_drawType;
		disease.nLevel = diseaseSetInfo.nLevel;
		disease.nRSurfaceType = diseaseSetInfo.nRoadSurfaceType;
		disease.diseaseWeight = diseaseSetInfo.fWidget;		//权重
		strcpy(disease.strRoadStandard, projectSetInfo.strRoadStandard);
		//计算病害中心里程
		disease.dMileage = this->caculateLittleFrameMiddleMile(this->m_tmpLittleFrameDiseaseRects);
		//开始里程
		disease.dDmiStart = this->calculateLittleFrameBeginMile(m_tmpLittleFrameDiseaseRects);
		//结束里程
		disease.dDmiEnd = this->calculateLittleFrameEndMile(m_tmpLittleFrameDiseaseRects);
	} 
	CalculateDiseaseSize(this->m_tmpLittleFrameDiseaseRects,disease);
	strcpy(disease.strDiseaseTableName, diseaseSetInfo.strDBTableName);
	strcpy(disease.strDisName, diseaseSetInfo.strDiseaseTypeName);
	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
	return disease;
}

vector<hn2dRectI> hn3dPixWidget::generateLittleFrameHn2dRectVector(const QVector<QRect>& rects)
{
	std::vector<hn2dRectI> dstVec;
	hn2dRectI hnRect;
	hn2d3dCoordinates tool;

	for (const QRect &rect : qAsConst(rects))
	{
		//用每个自动化模式的中心点来映射
		hn2dPointWithMileI p;
		std::vector<hn2dRectI> tmpVec;

		p = getHnPoint2dWithMileI(rect.center());
		tmpVec = tool.get2dLittleRects(p);
		dstVec.insert(dstVec.end(), tmpVec.begin(), tmpVec.end());
	}

	return dstVec;
}

vector<hn3dRectI> hn3dPixWidget::generateLittleFrameHn3dRectVector(const QVector<QRect>& rects)
{
	std::vector<hn3dRectI> dstVec;
	hn3dRectI hnRect;

	for (const QRect &rect : qAsConst(rects))
	{
		hnRect.p0 = this->getHnPoint3dWithMileI(rect.topLeft());
		hnRect.p1 = this->getHnPoint3dWithMileI(rect.topRight());
		hnRect.p2 = this->getHnPoint3dWithMileI(rect.bottomRight());
		hnRect.p3 = this->getHnPoint3dWithMileI(rect.bottomLeft());
		dstVec.push_back(hnRect);
	}

	return dstVec;
}



double hn3dPixWidget::caculateLittleFrameMiddleMile(const QVector<QRect>& rects)
{
	double mile;

	double beginMile = this->calculateLittleFrameBeginMile(rects);

	double endMile = this->calculateLittleFrameEndMile(rects);

	mile = (beginMile + endMile) / 2;

	return mile;
}

double hn3dPixWidget::calculateLittleFrameBeginMile(const QVector<QRect> rects)
{
	double beginMile = 0.0f;
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return beginMile;
	}
	//获取自动化模式的矩形数组中，最下面那条线的y值
	const int maxY = this->findMaxY(rects);
	beginMile = this->caculateEncoderMileByBigImagePoint(QPoint(0, maxY));

	return beginMile;
}

double hn3dPixWidget::calculateLittleFrameEndMile(const QVector<QRect> rects)
{
	double endMile = 0.0f;
	//异常处理
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return endMile;
	}
	//获取自动化模式的矩形数组中，最上面那条线的y值
	const int minY = this->findMinY(rects);
	endMile = this->caculateEncoderMileByBigImagePoint(QPoint(0, minY));

	return endMile;
}

QRect hn3dPixWidget::bigImageRectToSingleImageRect(const QRect & bigImageRect, QString * imageName)
{
	QPoint topLeft = this->bigImagePointToSingleImagePoint(bigImageRect.topLeft(), imageName);
	QPoint bottomRight = this->bigImagePointToSingleImagePoint(bigImageRect.bottomRight(), imageName);

	QRect singleImageRect(topLeft, bottomRight);
	return singleImageRect;
}

QRect hn3dPixWidget::singleImageRectToBigImageRect(const QRect & singleImageRect, const QString & imageName)
{
	QPoint topLeft = this->singleImagePointToBigImagePoint(singleImageRect.topLeft(), imageName);
	QPoint bottomRight = this->singleImagePointToBigImagePoint(singleImageRect.bottomRight(), imageName);

	QRect bigImageRect(topLeft, bottomRight);
	return bigImageRect;
}

QVector<QRect> hn3dPixWidget::calculateBigImageRects(const QString & imageName)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<QRect>();
	}

	QVector<QRect> dstRects;

	for (const QRect rect : qAsConst(this->m_singleImageLittleFrameRects))
	{
		QRect newRect = this->singleImageRectToBigImageRect(rect, imageName);
		dstRects.push_back(newRect);
	}
	return dstRects;
}


QVector<QRect> hn3dPixWidget::createSingleImageLittleFrameRect()
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<QRect>();
	}

	double roadWidth2d;
	double roadWidth3d;
	//获取三维路面宽度
	roadWidth3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	if (hnCommon::PROJECT_TYPE::PROJECT_23D_TYPE == m_projectType)
	{
		//获取二维路面宽度
		roadWidth2d = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	}

	//目标自动化模式数组
	QVector<QRect> dstRects;

	//算出每个像素代表多少米
	double widthScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	double heightScale = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();

	//矩形宽度（米） 每个小矩形宽度是10cm 也就是0.1米
	double rectWidth = 0.1;

	//算出自动化模式边长 单位：像素
	int widthSideLenth = this->caculateLittleFrameSideLenth(widthScale, rectWidth);
	int heightSideLenth = this->caculateLittleFrameSideLenth(heightScale, rectWidth);

	//三维图片像素宽度
	int image3dPixelWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelWidth();

	//三维图片像素高度
	int image3dPixelHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImagePixelHeight();

	//二维图片像素宽度 
	//这个是指在三维图片上截取的二维图片的像素宽度
	int image2dPixelMapWidth = image3dPixelWidth *(roadWidth2d / roadWidth3d);

	//横向矩形的个数
	int widthRectCount;

	//如果是二三维工程，就用二维的图片宽度来算横向自动化模式的个数
	if (hnCommon::PROJECT_TYPE::PROJECT_23D_TYPE == m_projectType)
	{
		widthRectCount = image2dPixelMapWidth / widthSideLenth;
	}
	//如果是单三维工程，就用三维的图片宽度来算横向自动化模式的个数
	else
	{
		widthRectCount = image3dPixelWidth / widthSideLenth;
	}

	//纵向矩形的个数
	int heightRectCount = image3dPixelHeight / heightSideLenth;

	//算出二三维工程，自动化模式向右偏移的像素值
	int leftSidePixel;
	if (hnCommon::PROJECT_TYPE::PROJECT_23D_TYPE == m_projectType)
	{
		leftSidePixel = image3dPixelWidth*(((roadWidth3d - roadWidth2d) / 2) / roadWidth3d);
	}

	//循环向目标的数据里面添加矩形
	QRect rect;					//单个自动化模式的矩形
	QPoint topLeftPoint;		//左上角的点
	QPoint bottomRightPoint;	//右下角的点
	for (int i = 0; i < heightRectCount; i++)
	{
		for (int j = 0; j < widthRectCount; j++)
		{
			if (hnCommon::PROJECT_TYPE::PROJECT_23D_TYPE == m_projectType)
			{
				//计算左上角的点
				topLeftPoint = QPoint(j * widthSideLenth + leftSidePixel, i * heightSideLenth);
				//计算右下角的点
				bottomRightPoint = QPoint(j * widthSideLenth + widthSideLenth + leftSidePixel, i * heightSideLenth + heightSideLenth);
			}
			else
			{
				//计算左上角的点
				topLeftPoint = QPoint(j * widthSideLenth, i * heightSideLenth);
				//计算右下角的点
				bottomRightPoint = QPoint(j * widthSideLenth + widthSideLenth, i * heightSideLenth + heightSideLenth);
			}

			//得到矩形
			rect = QRect(topLeftPoint, bottomRightPoint);
			//插入数组
			dstRects.push_back(rect);
		}
	}

	return dstRects;
}

QVector<QRect> hn3dPixWidget::createLittleFrameRects(QVector<pixImagePoint> pixImagePoints)
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

void hn3dPixWidget::addCtrlPoint(const QPoint & mousePoint)
{
	if (false == hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	if (nullptr == hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		return;
	}

	// 屏幕坐标转大image坐标
	QString pixName;
	QPoint singleImagePoint = this->screenToSingleImagePoint(mousePoint, pixName);
	QFileInfo pixInfo(pixName);
	// 转为不带路径、不含RGB/GREY的图片名称
	pixName = pixInfo.fileName();
	pixName.replace("GREY", "");
	pixName.replace("RGB", "");

	hnKZDDataInfo ctrlPoint;
	// ID
	ctrlPoint.nID = hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->getMaxID();
	// 控制点名称
	QString name = QString::fromLocal8Bit("KZD_%1").arg(ctrlPoint.nID);
	strcpy(ctrlPoint.strKzdName, name.toLocal8Bit().data());
	// 控制点GPS时间
	ctrlPoint.dGpsTimer = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getGpsTimer(pixName, m_pixHeight - singleImagePoint.y());
	// 里程
	ctrlPoint.dMileage = caculateEncoderMileByScreenPoint(mousePoint);
	// 图片名称
	strcpy(ctrlPoint.strImageName, pixName.toLocal8Bit().data());
	// 图片x
	ctrlPoint.nLocX = singleImagePoint.x();
	// 图片y
	ctrlPoint.nLocY = singleImagePoint.y();
	//三维坐标
	hn2dPointI point2d;
	point2d.x = singleImagePoint.x();
	point2d.y = m_pixHeight - singleImagePoint.y();
	hn3dPointD point3d;
	hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->get2DCoord(pixName, point2d, point3d);
	ctrlPoint.dX = point3d.x;
	ctrlPoint.dY = point3d.y;
	ctrlPoint.dZ = point3d.z;

	//写入数据库
	hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->writeData(ctrlPoint);
	//更新界面
	this->update();
}

void hn3dPixWidget::deleteCtrlPoint(const QPoint & mousePoint)
{
	this->hundleSelectCtrlPoint(mousePoint, &hn3dPixWidget::deleteCtrlPoint);
}

void hn3dPixWidget::deleteCtrlPoint(const hnKZDDataInfo & KZDDataInfo)
{
	int rc = QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("是否删除病害"),
		QString::fromLocal8Bit("是"), QString::fromLocal8Bit("否"));
	if (0 == rc)
	{
		hnDataManager::getDataManager()->getCurrentProject()->getDB()->getCtrlPointTable()->deleteData(KZDDataInfo);
		return;
	}
	else
	{
		return;
	}
}

void hn3dPixWidget::hundleSelectCtrlPoint(const QPoint & mousePoint, void(hn3dPixWidget::* hundleFunc)(const hnKZDDataInfo &KZDDataInfo))
{
	//转成大image坐标
	QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);

	for (auto ctrlPoint : m_currentCtrlPoints)
	{
		QString pixName = this->pixName3dConvertWithPath(ctrlPoint.strImageName);
		QPoint singelImagePoint(ctrlPoint.nLocX, ctrlPoint.nLocY);
		QPoint ctrlBigImagePoint = this->singleImagePointToBigImagePoint(singelImagePoint, pixName);
		QRect rect(ctrlBigImagePoint.x() - 20, ctrlBigImagePoint.y() - 20, 40, 40);

		if (rect.contains(bigImagePoint))
		{
			(this->*hundleFunc)(ctrlPoint);
			return;
		}
	}
}

void hn3dPixWidget::editCtrlPoint(const QPoint & mousePoint)
{
	this->hundleSelectCtrlPoint(mousePoint, &hn3dPixWidget::editCtrlPoint);
}

void hn3dPixWidget::editCtrlPoint(const hnKZDDataInfo & KZDDataInfo)
{
	ShowCtrlPointInfoDlg dialog;
	dialog.setName(QString::fromLocal8Bit(KZDDataInfo.strKzdName));
	dialog.setGpsTime(KZDDataInfo.dGpsTimer);
	dialog.setPixName(QString::fromLocal8Bit(KZDDataInfo.strImageName));
	dialog.setPixCoord(KZDDataInfo.nLocX, KZDDataInfo.nLocY);
	dialog.setPix3dCoord(KZDDataInfo.dX, KZDDataInfo.dY, KZDDataInfo.dZ);
	dialog.exec();
}

void hn3dPixWidget::drawCtrlPoint(QImage & image, const std::vector<hnKZDDataInfo> &ctrlPoints)
{
	QPainter painter(&image);
	QPen pen;
	pen.setColor(Qt::red);
	pen.setWidth(20);
	painter.setPen(pen);
	QFont font = painter.font();
	font.setBold(true);
	font.setPixelSize(m_fontSize);
	painter.setFont(font);

	for (auto ctrlPoint : qAsConst(ctrlPoints))
	{
		QString pixName = this->pixName3dConvertWithPath(ctrlPoint.strImageName);
		QPoint singelImagePoint(ctrlPoint.nLocX, ctrlPoint.nLocY);
		QPoint bigImagePoint = this->singleImagePointToBigImagePoint(singelImagePoint, pixName);

		painter.drawPoint(bigImagePoint);
		painter.drawText(bigImagePoint, QString::fromLocal8Bit(ctrlPoint.strKzdName));
	}

}

QString hn3dPixWidget::pixName3dConvertWithPath(const char * pixName)
{
	QString QStringPixName = QString::fromLocal8Bit(pixName);

	if (this->m_3dImageMode == hn3dImageMode::Gray)
	{
		QStringPixName = this->m_strGreyImaePath + "/GREY" + QStringPixName;
	}
	else
	{
		QStringPixName = this->m_strRGBImagePath + "/RGB" + QStringPixName;
	}

	return QStringPixName;
}

QString hn3dPixWidget::pixName3dConvertWithoutPath(const QString & pixName)
{
	QString newPixName;
	QFileInfo fileInfo(pixName);
	newPixName = fileInfo.fileName();
	newPixName.replace("GREY", "");
	newPixName.replace("RGB", "");
	return newPixName;
}

QRect hn3dPixWidget::drawTmpLittleBigFrameDisease(QImage &image)
{
	//把单张图片的坐标转成大张图片的坐标
	QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
	QPoint bigImageEnd = this->singleImagePointToBigImagePoint(m_diseaseEndPoint.pixPoint, m_diseaseEndPoint.pixName);


	QRect rect(bigImageStart, bigImageEnd);
	rect = rect.normalized();
	return rect;
}

bool hn3dPixWidget::isTmpDiseaseAreaValid(const hnCommon::hnRoadDiseaseInfo& disease)
{
	if (strcmp(disease.strRoadStandard, "低等级农村公路") == 0)
	{
		if (strcmp(disease.strDiseaseTableName, "DisLG") == 0 ||
			strcmp(disease.strDiseaseTableName, "DisSS") == 0)
		{
			if (disease.dArea < 20)
			{
				return false;
			}
		}
	}
	return true;
}

void hn3dPixWidget::mouseDoubleClickEvent(QMouseEvent *event)
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
			this->bigFrameEditProcess(event->pos());
		}
	}
}

bool hn3dPixWidget::isValidArea(QMouseEvent * event)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return false;
	}
	if (!hnApp::hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		return false;
	}

	if (m_pixNameMap.empty())
	{
		return false;
	}
	//cwb 20240909
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

hnCommon::hnRoadDiseaseInfo hn3dPixWidget::getMousePosDisease(const QPoint & mousePoint)
{
	if (m_frameMode == FrameMode::LITTLE_FRAME)
	{
		//获取鼠标点所在的自动化模式数组
		QPoint bigImagePoint = this->screenPointToBigImagePoint(mousePoint);
		//获取鼠标位置的病害
		for (auto disease : this->m_currentWidgetDiseases)
		{
			if (disease.vec3dRect.empty())
			{
				continue;
			}

			//获取病害的三维数组
			std::vector<hn3dRectI>  hn3dRects = disease.vec3dRect;
			QVector<QRect> diseaseRects;

			for (hn3dRectI rect3d : qAsConst(hn3dRects))
			{
				QRect rect = this->hn3dRectToBigImageQtRect(rect3d);
				diseaseRects.push_back(rect);
			} 
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
			if (disease.vec3dRect.empty())
			{
				continue;
			}
			hn3dRectI  hnRect = disease.vec3dRect.at(0);

			QRect diseaseRect = this->hn3dRectToBigImageQtRect(hnRect);

			QPoint imagePoint = this->screenPointToBigImagePoint(mousePoint);
			if (diseaseRect.contains(imagePoint))
			{
				return disease;
			}
		}
	}

	return hnRoadDiseaseInfo();
}

void hn3dPixWidget::selectDisease(const QPoint & mousePoint)
{
	//选中病害 病害列表选中
	auto disease = getMousePosDisease(mousePoint);
	if (disease.isValid())
	{
		emit	signal_selectDisease(disease);

	}
}

void hn3dPixWidget::slotDiseaseChanged()
{
	this->m_currentWidgetDiseases.clear();
	this->update();
}

