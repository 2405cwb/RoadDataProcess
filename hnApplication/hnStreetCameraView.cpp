#include <QPainter>
#include <QResizeEvent>
#include <QApplication>
#include <QDir>
#include <QRubberBand>
#include <QtDebug>
#include <vector>
#include<algorithm>
#include <iostream>
#include <QFileDialog>
#include <QDesktopWidget>
#include <thread>
#include <QMutexLocker>
#include "io.h"
#include "string.h"
#include "qmath.h"
#include "hnCommandDef.h"
#include "hnDataManager.h"
#include "hnStreetCameraView.h"
#include "hnImageBrightness.h"
#include "hnImageDistanceNavigation.h"
#include "..\hnProject\hn2DProject.h"
#include "..\hnProject\hnProject.h"
#include "hnDiseaseService.h"
//////////////////////////////////////////////////////////////////////////
using namespace hnPro;

namespace hnApp
{
	hnStreetCameraView::hnStreetCameraView(QWidget *parent)//m_scale(0.25)
		: hnView(parent), m_pCenter(0, 0), m_scale(0.12), m_LoadPic(NULL),
		m_bPushMiddleButton(false), m_bShow(true), m_nCurImageDmi(0),
		m_bFirstLoadImage(true), m_pictureInterval(0.0), m_calculateRoadWidthMode(false)
	{
		//背景
		QPalette myPalette = QPalette(palette());
		myPalette.setColor(QPalette::Background, QColor(33, 40, 48));
		setAutoFillBackground(true);
		setPalette(myPalette);

		//2020.8.17隐藏
		//this->setMinimumHeight(150);
		//this->setMinimumWidth(400);

		setContextMenuPolicy(Qt::CustomContextMenu);  // 设置右键菜单

													  //鼠标追踪
		setFocusPolicy(Qt::FocusPolicy::ClickFocus);
		setMouseTracking(true);

		//视图初始化  将类与本地消息重构 击穿 首次最难击穿 得到视图左上角原点屏幕坐标进行偏移  客户区域转屏幕区域  客户区域是除去标题栏之后的区域左上角为0,0  视图是屏幕左上角0,0
		POINT pt_view;
		pt_view.x = 0; pt_view.y = 0;
		ClientToScreen((HWND)this->winId(), &pt_view);

		//2020.8.17修正初始放缩值
		m_scale = 0.18;

		//图片中心设置
		m_pCenter = QPoint(0.0, 0.0);

		//中键点击点
		m_pMidClick = QPoint(0, 0);

		m_dOffsetX = 0.0;
		m_dOffsetY = 0.0;

		m_nStreetViewType = STREET_LEFT_VIEW;

		//添加病害模式
		m_workMode = WorkMode::ADD_MODE;
		m_originalWidgetWidth = 320;
		m_originalWidgetHeight = 240;
		m_pictureInterval = 1.0;
		connect(hnApp::hnDataManager::getDataManager()->getDiseaseService(),
			SIGNAL(diseaseChanged()), this, SLOT(slotStreetDiseaseChanged()));
	}

	hnStreetCameraView::~hnStreetCameraView()
	{
		if (m_LoadPic)
		{
			delete m_LoadPic;
			m_LoadPic = NULL;
		}

	}

	/************************************************************************/
	/*                            事件                              */
	/************************************************************************/

	// 绘制事件
	void hnStreetCameraView::paintEvent(QPaintEvent *)
	{
		if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
		{
			return ;
		} 
		if (m_nCurImageDmi < 0)
		{
			return;
		}

		QPainter painterTrans(this);
		QPainter painter(this);
	
		/*if (m_listImage.size() > m_nCurImageIndex)
		{
			QString pixName;
			pixName = m_listImage.at(m_nCurImageIndex);
			QImage image(pixName);
			if (true == image.isNull())
			{
				return;
			}
			*m_LoadPic = QPixmap::fromImage(image);
		}*/

		//没有图片
		if (!m_LoadPic||m_LoadPic->isNull())
		{
			return;
		} 

		// 设置偏移矩阵
     	QTransform transform;

		transform.translate(m_dOffsetX, m_dOffsetY);
		painterTrans.setWorldTransform(transform);

		hnMile currentHnMile;
		//获取当前的hnMle
		if (currentImageIndex() >= 0)
		{
			auto iter = m_pixPathStreetMilesMap.find(currentImagePath());
			if (iter != m_pixPathStreetMilesMap.end())
			{
				currentHnMile = iter.value();
			}
		}
		
		QVector<hnMile> currentMiles;
		currentMiles.push_back(currentHnMile);
		//获取病害
		QVector<hnRoadDiseaseInfo> diseases;
		 
		 hnApp::hnDataManager::getDataManager()->getDiseaseService()->getStreetDiseaseInRange(
		currentHnMile.dEnclMile, currentHnMile.dEnclMile,diseases
		);
		  
		//储存当前视图的病害
		// 自定义标注按原始相机侧显示，清空当前视图也只操作这一侧。
		for (int i = diseases.size() - 1; i >= 0; i--)
		{
			if (diseases.at(i).ndiseaseType == 3 &&
				(diseases.at(i).nDrawType == 11) != (m_nViewType != STREET_LEFT_VIEW)) diseases.removeAt(i);
		}
		this->m_currentWidgetDiseases = diseases;
			// 每次从无标注图重绘，避免病害文字反复叠加。
			*m_LoadPic = QPixmap::fromImage(m_sourceImageWithoutDisease);
			QPainter diseasePainter(m_LoadPic);
			QFont font = diseasePainter.font();
			font.setBold(true);
			const int fontSize = 90;
			font.setPixelSize(fontSize);
			QPen pen;
			pen.setColor(Qt::red);
			diseasePainter.setPen(pen);
			diseasePainter.setFont(font);

			m_currentDiseasesVector.clear();

			//往图片上画病害
			int diseaseY = fontSize;
			for (auto disease : qAsConst(diseases))
			{
				//文字 桩号+名字+扣分
				double region = currentHnMile.dTrueMile;
				double decimalPart = fmod(region, 1000);
				int integerPart = region / 1000;

				QString regionWithK = "K" + QString::number(integerPart) + "+" +
					QString::number(decimalPart, 'f', 3).rightJustified(7, '0');
				//病害分数
				int score = 0;
				hnDiseaseSetInfo setInfo;
				if (disease.ndiseaseType != 3 && hnDataManager::getDataManager()->getStreetDiseaseSetInfo(
					QString::fromLocal8Bit(disease.strDisName), currentHnMile.roadStandard, setInfo))
				{
					score = disease.dArea * setInfo.nDWKF;
				}

				QString text = QString::fromLocal8Bit("%1,%2,扣分：%3")
					.arg(regionWithK)
					.arg(QString::fromLocal8Bit(disease.strDisName))
					.arg(score);

				if (disease.ndiseaseType == 3)
				{
					text = QStringLiteral("%1，自定义：%2，数量：%3 %4").arg(regionWithK)
						.arg(QString::fromLocal8Bit(disease.strDisName)).arg(disease.dArea)
						.arg(QString::fromLocal8Bit(disease.strRemark));
					if (!disease.vec2dRect.empty())
					{
						const auto& rect = disease.vec2dRect.front();
						diseasePainter.drawRect(QRect(QPoint(rect.p0.x, rect.p0.y), QPoint(rect.p2.x, rect.p2.y)));
					}
				}
				diseasePainter.drawText(QPoint(0, diseaseY), text);

				diseaseY += fontSize;
			}

			//向窗口上绘制图片
			diseasePainter.end();
			drawImage(&painterTrans);

			if (m_calculateRoadWidthMode)
			{

				if (m_state == MeasureState::Idle)
				{

				}
				else
				{
					 
					painterTrans.setRenderHint(QPainter::Antialiasing, true);
					QPointF p1 = m_startPoint;
					QPointF p2;
					if (m_state == MeasureState::Measuring)
					{
						p2 = m_currentPoint;
					}
					else
					{
						p2 = m_endPoint;
					}
					QPen pointPen(Qt::red);
					pointPen.setWidth(6);
					painterTrans.setPen(pointPen);
					painterTrans.drawPoint(p1);
					painterTrans.drawPoint(p2);

					QPen linePen(Qt::red);
					linePen.setWidth(2);
					if (m_state == MeasureState::Measuring)
					{
						linePen.setStyle(Qt::DashLine);
					}
					else
					{
						linePen.setStyle(Qt::SolidLine);
					}
					painterTrans.setPen(linePen);
					painterTrans.drawLine(p1, p2);
					QLineF line(p1, p2);
					double pixlLength = line.length();
				//	double realLength = pixlLength * m_unitPerPixel;
					QSize labelSize = this->size();
					QSize imageSize = m_LoadPic->size();
					double scaleX = static_cast<double> (imageSize.width()) / labelSize.width();
					double scaleY = static_cast<double> (imageSize.height()) / labelSize.height();
					QPoint pictureP1, pictureP2;
					pictureP1.setX(p1.toPoint().x() * scaleX);
					pictureP1.setY(p1.toPoint().y() *scaleY);
					pictureP2.setX(p2.toPoint().x() * scaleX);
					pictureP2.setY(p2.toPoint().y() *scaleY);
					double realLength = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->caculateStreetLength(pictureP1, pictureP2);
					QString text = QString::number(realLength, 'f', 2) + "" + m_unitName;


					QPointF mid = (p1 + p2) / 2.0;
					QPointF textPos = mid + QPointF(10, -10);

					QFontMetrics fm(painter.font());
					QRect textRect = fm.boundingRect(text);
					textRect.adjust(-6, -4, 6, 4);
					textRect.moveTopLeft(textPos.toPoint());

					painterTrans.setPen(Qt::NoPen);
					painterTrans.setBrush(QColor(255, 255, 255, 220));
					painterTrans.drawRoundedRect(textRect, 4, 4);
					painterTrans.setPen(Qt::black);
					painterTrans.drawText(textRect, Qt::AlignCenter, text);

				}

			}

	}

	// 窗体大小改变事件
	void hnStreetCameraView::resizeEvent(QResizeEvent * e)
	{
		if (abs(e->oldSize().height() - 0.0) < 0.0001)
		{
			m_scale = 0.12;
			return;
		}

		if (m_LoadPic != NULL)
		{
			int labelWidth = e->size().width();
			int labelHeight = e->size().height();
			int imgWidth = m_LoadPic->width();
			int imgHeight = m_LoadPic->height();

			double scalex = e->size().width() / (double)(m_LoadPic->width());
			double scaley = e->size().height() / (double)(m_LoadPic->height());

			m_scale = scalex < scaley ? scalex : scaley;

			// 更新中心位置;
			int nLeft = (labelWidth - imgWidth*m_scale) / 2.0;
			int nUp = (labelHeight - imgHeight*m_scale) / 2.0;
			m_pCenter = QPoint(nLeft, nUp);
		}
		else
		{
			double scale = e->size().height() / (double)e->oldSize().height();
			if (scale > 0)
			{
				m_scale = m_scale * scale;
			}
		} 
	}

	// 鼠标滚动事件(放大缩小)
	void hnStreetCameraView::stepImage(int step)
	{
		if (currentImageIndex() < 0)
		{
			return;
		}
		const hnImageDistanceNavigation navigation;
		const double target = navigation.stepDistance(m_nCurImageDmi, m_pictureInterval, m_listImage.size(), step);
		emit updateShowImg(target);
	}

	void hnStreetCameraView::wheelEvent(QWheelEvent *e)
	{
		if (e->delta() != 0)
		{
			stepImage(e->delta() > 0 ? 1 : -1);
		}
		e->accept();
	}

	void hnStreetCameraView::mousePressEvent(QMouseEvent *event)
	{
		if (!hnDataManager::getDataManager()->isOpenProject())
		{ 
			return;
		}
		if (m_calculateRoadWidthMode)
		{
			if (event->button() == Qt::MouseButton::RightButton)
			{
				cancelMeasure();
			}
			if (event->button() != Qt::MouseButton::LeftButton)
			{
				return;
			}
			QPointF pos = event->pos();
			if (m_state == MeasureState::Idle || m_state ==  MeasureState::Finished)
			{
				m_startPoint = pos;
				m_currentPoint = pos;
				m_endPoint = pos;
				m_state = MeasureState::Measuring;
			}
			else if(m_state == MeasureState::Measuring)
			{
				m_endPoint = pos;
				m_currentPoint = pos;
				m_state = MeasureState::Finished;
				 
			}
			update(); 
			return;
		}
		

	#if 0	//禁用鼠标中键移动图片
		if (event->button() == Qt::MidButton)
		{
			QPoint oldPos = event->pos();
			getOldCenterByMoveImage(oldPos.x(), oldPos.y());

			m_bPushMiddleButton = true;
		}
	#endif

		//添加病害
		if (this->m_workMode == hnWorkMode::ADD_MODE && event->button() == Qt::MouseButton::LeftButton)
		{
			this->addStreetDisease();
		}
		//删除病害
		else if (this->m_workMode == hnWorkMode::DELETE_MODE && event->button() == Qt::MouseButton::LeftButton)
		{
			this->deleteStreetDisease();
		}
	}

	// 鼠标弹起事件
	void hnStreetCameraView::mouseReleaseEvent(QMouseEvent *event)
	{
		m_bPushMiddleButton = false;
	}

	// 鼠标移动事件
	void hnStreetCameraView::mouseMoveEvent(QMouseEvent *event)
	{
		if (m_calculateRoadWidthMode)
		{
			if (m_state == MeasureState::Measuring)
			{
				m_currentPoint = event->pos();
				update();
			}
			return;
		}
#if 0
		if (m_bPushMiddleButton)
		{
			QPoint oldPos = event->pos();
			getNewCenterByMoveImage(oldPos.x(), oldPos.y());
		}

		if (m_LoadPic == NULL)
		{
			return;
		}

		update();
#endif

		// m_nCurImageDmi is a travelled distance, not a list index. Use the
		// already loaded clean image so all frames and rotations share paint coordinates.
		if (!m_sourceImageWithoutDisease.isNull())
		{
			QImage originalImage = this->getOriginalImage(event->pos(),
				m_sourceImageWithoutDisease, m_originalWidgetWidth, m_originalWidgetHeight);
			emit sig_mousePosImageChanged(originalImage);
		}

	}

	// 键盘按下事件 空格键 浏览模式 图片回到正常水平
	void hnStreetCameraView::keyPressEvent(QKeyEvent *e)
	{
		switch (e->key())
		{
		case Qt::Key_Left:
		case Qt::Key_Up:
			stepImage(-1);
			break;
		case Qt::Key_Right:
		case Qt::Key_Down:
			stepImage(1);
			break;
		default:
			e->ignore();
			return;
		}
		e->accept();
	}

	/************************************************************************/
	/*                            fun                              */
	/************************************************************************/

	// 自定义坐标转换屏幕坐标
	QPoint hnStreetCameraView::fromUser2Screen(const QPoint& pti)
	{
		QPoint pt;

		pt.rx() = pti.x() * m_scale + m_pCenter.y();
		pt.ry() = pti.y() * m_scale + m_pCenter.x();

		QTransform transform;
		transform.translate(m_dOffsetX, m_dOffsetY);

		pt = transform.map(pt);

		return pt;
	}

	// 屏幕坐标转换为自定义坐标
	QPoint hnStreetCameraView::fromScreen2User(const QPoint& pti)
	{
		QPoint pt;
		pt.rx() = pti.x();
		pt.ry() = pti.y();

		QTransform transform;
		transform.translate(m_dOffsetX, m_dOffsetY);
		transform = transform.inverted();

		pt = transform.map(pt);

		pt.rx() = (pt.x() - m_pCenter.y()) / m_scale + 0.5;
		pt.ry() = (pt.y() - m_pCenter.x()) / m_scale + 0.5;

		return pt;
	}

	void hnStreetCameraView::setUnitScale(double unitPerPixel, const QString& unitName)
	{
		m_unitPerPixel = unitPerPixel;
		m_unitName = unitName;
		update();
	}

	//获得原始图片中心 鼠标中键按下 用于移动图片
	void hnStreetCameraView::getOldCenterByMoveImage(int X, int Y)
	{
		m_pMidClick = QPoint(X, Y) - m_pCenter;
	}

	//获得新的移动点 鼠标中键移动 用于移动图片
	void hnStreetCameraView::getNewCenterByMoveImage(int X, int Y)
	{
		QPoint pCenter;
		pCenter = QPoint(X, Y);

		QPoint MidClickPoint = m_pMidClick;

		QPoint tempPt = pCenter - MidClickPoint;

		m_pCenter = tempPt;

		update();

	}

	//放大缩小图片
	void hnStreetCameraView::scaleImage(short zDelta)
	{
		double dScale = m_scale;

		if (zDelta > 0.0)
		{
			dScale += 0.05;
		}
		else
		{
			dScale -= 0.05;

			//最小为0.09
			if (dScale < 0.09)
			{
				return;
			}

			if (dScale < 0.0)
			{
				dScale += 0.05;
				return;
			}
		}

		//小于该值
		if (dScale <= 0.034)
		{
			return;
		}

		//设置值
		m_scale = dScale;

	}

	/************************************************************************/
	/*                                public                                 */
	/************************************************************************/

	// 添加图片 bResetCurImage表示重置当前图片
	bool hnStreetCameraView::addStreetImage(bool needRotate,const QString& picPath)
	{
		// 同一张图只解码一次，亮度调节和病害刷新复用未处理原图。
		if (m_originalStreetImage.isNull() || m_curPicPath != picPath || m_needRotate != needRotate)
		{
			QImage original(picPath);
			if (original.isNull())
			{
				m_originalStreetImage = QImage();
				m_sourceImageWithoutDisease = QImage();
				if (m_LoadPic)
				{
					*m_LoadPic = QPixmap();
				}
				update();
				return false;
			}
			if (needRotate)
			{
				QTransform transform;
				transform.rotate(90);
				original = original.transformed(transform, Qt::SmoothTransformation);
			}
			m_originalStreetImage = original;
			m_needRotate = needRotate;
			m_curPicPath = picPath;
			refreshBrightness();
		}
		//图片大小设置
		if (m_bFirstLoadImage)
		{
			int labelWidth = this->size().width();
			int labelHeight = this->size().height();
			int imgWidth = m_LoadPic->width();
			int imgHeight = m_LoadPic->height();

			double scalex = this->size().width() / (double)(m_LoadPic->width());
			double scaley = this->size().height() / (double)(m_LoadPic->height());

			m_scale = scalex < scaley ? scalex : scaley;

			// 更新中心位置;
			int nLeft = (labelWidth - imgWidth*m_scale) / 2.0;
			int nUp = (labelHeight - imgHeight*m_scale) / 2.0;
			m_pCenter = QPoint(nLeft, nUp);
			//emit updateShowImg(-1);
			m_bFirstLoadImage = false;
		}

		//设置鼠标的形态  十字形态
		this->setCursor(Qt::CrossCursor);

		//更新
		updateDrawData();

		return true;

	}

	// 添加图片
	bool hnStreetCameraView::addImage(bool needRotate, double nImageIndex)
	{
		const hnImageDistanceNavigation navigation;
		const int index = navigation.imageIndex(nImageIndex, m_pictureInterval, m_listImage.size());
		if (index < 0)
		{
			return false;
		}
		m_nCurImageDmi = qMax(0.0, nImageIndex);
		return addStreetImage(needRotate, m_listImage.at(index));
	}

	int hnStreetCameraView::currentImageIndex() const
	{
		const hnImageDistanceNavigation navigation;
		return navigation.imageIndex(m_nCurImageDmi, m_pictureInterval, m_listImage.size());
	}

	QString hnStreetCameraView::currentImagePath() const
	{
		return m_listImage.value(currentImageIndex());
	}

    bool hnStreetCameraView::currentDisplayedImageInfo(QString* imagePath, double* trueMile) const
    {
        if (m_originalStreetImage.isNull() || m_curPicPath.isEmpty() ||
            m_curPicPath != currentImagePath()) return false;
        if (imagePath) *imagePath = m_curPicPath;
        if (trueMile)
        {
            auto iter = m_pixPathStreetMilesMap.constFind(m_curPicPath);
            if (iter != m_pixPathStreetMilesMap.constEnd())
            {
                *trueMile = iter.value().dTrueMile;
            }
            else
            {
                const auto manager = hnDataManager::getDataManager();
                const auto project = manager ? manager->getCurrentProject() : nullptr;
                if (!project || currentImageIndex() < 0) return false;
                *trueMile = project->enclToTrueMile(currentImageIndex() * m_pictureInterval);
            }
        }
        return true;
    }

	void hnStreetCameraView::clear()
	{

		//清空图片
		m_sourceImageWithoutDisease = QImage();
		m_originalStreetImage = QImage();
		m_listImage.clear();
		m_pixPathStreetMilesMap.clear();
		m_streetMiles.clear();
		m_nCurImageDmi = 0.0;
		m_curPicPath.clear();
		if (m_LoadPic)
		{
			delete m_LoadPic;
			m_LoadPic = NULL;
		}

		//图片大小设置
		m_pCenter = QPoint(0.0, 0.0);

		//更新
		update();

	}

	//ptInImage
	bool hnStreetCameraView::ptInImage(const QPoint& p)
	{
		if (m_LoadPic == NULL)
		{
			return false;
		}

		if (p.x() < 0 || p.y() < 0 || p.x() >= m_LoadPic->width() || p.y() >= m_LoadPic->height())
		{
			return false;
		}

		return true;
	}

	// 绘制图片
	void hnStreetCameraView::drawImage(QPainter * painter)
	{
		painter->save();

		//重新画图
	//	QRect rect;

	//	rect.setRect(m_pCenter.x(), m_pCenter.y(), m_LoadPic->width() * m_scale, m_LoadPic->height() * m_scale);

	//painter->drawPixmap(rect, *m_LoadPic);
	painter->drawPixmap(rect(), m_LoadPic->scaled(size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
	 
		painter->restore();

	}


	void hnStreetCameraView::cancelMeasure()
	{
		m_state = MeasureState::Idle;
		update();

	}

	// 设置显示状态
	void hnStreetCameraView::setShowState(bool bShow)
	{
		m_bShow = bShow;

		update();
	}


	// 更新绘制数据
	void hnStreetCameraView::updateDrawData()
	{

		update();
	}

	// 重新加载数据
	void hnStreetCameraView::reloadData(bool needRotate, STREET_VIEW_TYPE nViewType)
	{
		m_nViewType = nViewType;
		m_pixPathStreetMilesMap.clear();
		//获取景观的桩号
		if (nViewType ==  STREET_LEFT_VIEW)
		{
			m_streetMiles = hnDataManager::getDataManager()->getCurrentProject()->getLeftStreetMiles();

		}
		else
		{
			m_streetMiles = hnDataManager::getDataManager()->getCurrentProject()->getRightStreetMiles(); 
		}

		for (auto mile : qAsConst(m_streetMiles))
		{
			if (nViewType == STREET_LEFT_VIEW)
			{
				m_pixPathStreetMilesMap.insert(mile.leftStreetPicPath, mile);
			}
			else
			{
				m_pixPathStreetMilesMap.insert(mile.rightStreetPicPath, mile);
			}		
		}

		if (nViewType == STREET_LEFT_VIEW)
		{
			m_listImage = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getLeftStreetPicturePath();
		}
		else
		{
			m_listImage = hnDataManager::getDataManager()->getCurrentProject()->get2DProject()->getRightStreetPicturePath();
		}

		m_nCurImageDmi = 0;

		if (m_listImage.size() <= 0)
		{
			return;
		}
		if (nViewType== STREET_LEFT_VIEW)
		{
			addStreetImage(false,m_listImage.first());

		}
		else
		{
			addStreetImage(needRotate, m_listImage.first());


		}

	}

	void hnStreetCameraView::setStreetInterval(double interval)
	{
		m_pictureInterval = interval;
	}

	void hnStreetCameraView::slotStreetDiseaseChanged()
	{
		addStreetImage(m_needRotate, m_curPicPath);
		//update();
	}

	void hnStreetCameraView::slot_updatePictureBrightness(int value)
	{

		m_brightnessValue = qBound(-100, value, 300);
		refreshBrightness();
	}

	//添加病害
	void hnStreetCameraView::addStreetDisease()
	{
		if (m_calculateRoadWidthMode)
		{
			return;
		}
		if (m_listImage.empty())
		{
			return;
		}
		if (this->m_pixPathStreetMilesMap.size()==0)
		{
			return;
		}
		//获取图片的hnMile
		//获取图片的hnMile
		int mileIdx = currentImageIndex();
		if (mileIdx < 0)
		{
			return;
		}
		QString key = m_listImage.at(mileIdx);
		auto it =  this->m_pixPathStreetMilesMap.find(key);
		if (it==this->m_pixPathStreetMilesMap.end())
		{
			return;
		}
		hnMile& currentMile = it.value();
		//添加景观对话框弹出
		auto ljInfo = hnDataManager::getDataManager()->getCurrentProjectStreetDiseases(currentMile, 2);
		auto yxInfo = hnDataManager::getDataManager()->getCurrentProjectStreetDiseases(currentMile, 1);
		hnAddStreetDiseaseDialog dialog(ljInfo, yxInfo, this);
		dialog.setCurrentHnMile(currentMile);
		dialog.setImageSide(m_nViewType == STREET_LEFT_VIEW ? 0 : 1);
		if (dialog.exec() == QDialog::Accepted)
		{
			update();
			auto diseases = dialog.getSelectDiseases();
			for (auto disease : qAsConst(diseases))
			{
				//hnApp::hnDataManager::getDataManager()->getDiseaseService()->addDisease(disease);
				//emit signal_addDisease(disease,false);
			}
		}
		return;
	}

	void hnStreetCameraView::deleteStreetDisease()
	{
		const int  reply =
			QMessageBox::question(nullptr, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("是否清空当前视图的病害？"),
				QString::fromLocal8Bit("是"),QString::fromLocal8Bit("否"));
		if (0 == reply)
		{		 
			for (auto disease : qAsConst(m_currentWidgetDiseases))
			{
				hnApp::hnDataManager::getDataManager()->getDiseaseService()->deleteOneDisease(disease);
			}
		}
		else 
		{
			return;
		}
	}

	void hnStreetCameraView::deleteStreetDisease(const QPoint & point)
	{
		//this->mapTo(m_LoadPic, point);
	}

	void hnStreetCameraView::refreshBrightness()
	{
		if (m_originalStreetImage.isNull())
		{
			return;
		}
		const hnImageBrightness brightness;
		m_sourceImageWithoutDisease = brightness.adjust(m_originalStreetImage, m_brightnessValue);
		if (!m_LoadPic)
		{
			m_LoadPic = new QPixmap;
		}
		*m_LoadPic = QPixmap::fromImage(m_sourceImageWithoutDisease);
		updateDrawData();
	}

	QImage  hnStreetCameraView::getOriginalImage(QPoint mousePos, const QImage &tmpImageWithoutDisease, const int originalWidgetWidth, const int originalWidgetHeight)
	{
		//获取鼠标的坐标
		QPoint mousePosPoint = mousePos;

		//获取到显示图片label的image
		QImage srcImage = tmpImageWithoutDisease;

		//鼠标坐标转换为iamge中的坐标 //这个1.0必须要加,不然会有误差,下同
		mousePosPoint.setX(mousePosPoint.x() * (srcImage.width()* 1.0 / this->width()));
		mousePosPoint.setY(mousePosPoint.y() * (srcImage.height()* 1.0 / this->height()));

		//截取局部图片
		QImage dstImage;

		//左上角坐标
		QPoint leftTopPoint;
		//右下角坐标
		QPoint rightButtonPoint;

		int leftTopX = 0, leftTopY = 0, rightButtonX = 0, rightButtonY = 0;

		leftTopX = mousePosPoint.x() - originalWidgetWidth / 2.0;
		leftTopY = mousePosPoint.y() - originalWidgetHeight / 2.0;
		rightButtonX = mousePosPoint.x() + originalWidgetWidth / 2.0;
		rightButtonY = mousePosPoint.y() + originalWidgetHeight / 2.0;

#pragma region 越界控制

		//做一下判断，要是越界就控制一下

		//如果左上角的x小于0  
		if (leftTopX < 0)
		{
			leftTopX = 0;	//左上角的x就等于0
			rightButtonX = originalWidgetWidth;//右下角的x等于原始比例显示窗口的宽
		}

		//如果右下角的y大于image的最大值
		if (rightButtonY > srcImage.height())
		{
			rightButtonY = srcImage.height();
			leftTopY = srcImage.height() - originalWidgetHeight;
		}

		//如果左上角的y小于0
		if (leftTopY < 0)
		{
			leftTopY = 0;
			rightButtonY = originalWidgetHeight;
		}

		//如果右下角的x大于 image的宽的最大值
		if (rightButtonX > srcImage.width())
		{
			rightButtonX = srcImage.width();
			leftTopX = srcImage.width() - originalWidgetWidth;
		}
#pragma endregion

		//设置点的xy
		leftTopPoint.setX(leftTopX);
		leftTopPoint.setY(leftTopY);
		rightButtonPoint.setX(rightButtonX);
		rightButtonPoint.setY(rightButtonY);

		//截图
		dstImage = srcImage.copy(QRect(leftTopPoint, rightButtonPoint));

		//返回image
		return dstImage;
	}
	void hnStreetCameraView::setOriginalWidgetWidthHeight(const int w, const int h)
	{
		m_originalWidgetWidth = w;
		m_originalWidgetHeight = h;
	}

}

