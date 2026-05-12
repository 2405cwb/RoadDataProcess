#include "hn2d3dPixBaseWidget.h"
#include <QTimer>


hn2d3dPixBaseWidget::hn2d3dPixBaseWidget(QWidget *parent) : hnBrowsePixWidget(parent)
{
	this->m_workMode = WorkMode::NO_MODE;
	this->m_isRightDeleteMouseDown = false;
	this->m_isEndAddPoint = false;
}


hn2d3dPixBaseWidget::~hn2d3dPixBaseWidget()
{
}

void hn2d3dPixBaseWidget::keyPressEvent(QKeyEvent * event)
{
	//Qt::Key_Enter是小键盘的回车  Qt::Key_Return 是大键盘的回车
	if ((Qt::Key_Enter == event->key() || Qt::Key_Return == event->key())
		&& FrameMode::DESIGN_LINE == m_frameMode && WorkMode::ADD_MODE == m_workMode)
	{
		if (m_isDrawingDisease)
		{
		
			 for (int i = 0 ; i<	m_tmpPaintLineDiseasePoints.size();++i)
			 {
				 for (int j = 0 ; j<m_tmpLastPaintLineDiseasePoints.size();++j)
				 {
					 if (m_tmpPaintLineDiseasePoints[i] == m_tmpLastPaintLineDiseasePoints[j])
					 {
						 m_tmpPaintLineDiseasePoints.remove(i);
						 --i;
						 break;
					 }
				 }
			 }
			this->lineDiseaseAddDisease();
		}
	}

	// 按下 B键 时，切换线状病害添加模式(单个自动化模式绘制模式下生效)
	if (event->key() == Qt::Key_B && !this->littleDrawRectType)
	{
		this->addLineDiseType = !this->addLineDiseType;
		this->m_isDrawingDisease = false;

		m_diseaseStartPoint.pixPoint = QPoint(-1, -1);
		m_diseaseEndPoint.pixPoint = QPoint(-1, -1);
		m_diseaseAddPoint.pixPoint = QPoint(-1, -1);
		m_littleSingleImagePoints.clear();
		m_litteBigImagePoints.clear();
		m_tmpLittleFrameDiseaseRects.clear();
	}

	QWidget::keyPressEvent(event);
}

void hn2d3dPixBaseWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::RightButton
		&& FrameMode::DESIGN_LINE == m_frameMode && WorkMode::ADD_MODE == m_workMode)
	{
		if (m_isDrawingDisease)
		{

			for (int i = 0; i < m_tmpPaintLineDiseasePoints.size(); ++i)
			{
				for (int j = 0; j < m_tmpLastPaintLineDiseasePoints.size(); ++j)
				{
					if (m_tmpPaintLineDiseasePoints[i] == m_tmpLastPaintLineDiseasePoints[j])
					{
						m_tmpPaintLineDiseasePoints.remove(i);
						--i;
						break;
					}
				}
			}
			this->lineDiseaseAddDisease();
			m_isDrawingDisease = false;
		} 
	}
	QWidget::mousePressEvent(event);
}

void hn2d3dPixBaseWidget::slot_cancelDrawDiseases()
{
	this->m_isDrawingDisease = false;
	this->m_isAllowDrawPix = true;
	this->m_isAllowLinked = true;
	//清空合并病害的数组
	m_seclectedDiseases.clear();
	m_tmpLineDiseasePoints.clear();
	m_tmpPaintLineDiseasePoints.clear();
	this->update();
}



 

void hn2d3dPixBaseWidget::slot_deleteDisease(const hnRoadDiseaseInfo &disease)
{

}

void hn2d3dPixBaseWidget::slot_moveMouse(bool up, bool is2D)
{
	//用户按下上下键后 需要先移动图片 然后控制鼠标纵向移动一定范围 
	//在滚轮滚动的时候，允许画病害图片
	this->m_isAllowDrawPix = true;
	if (this->m_workMode == WorkMode::ADD_MODE&&
		this->m_isDrawingDisease
		&& this->m_frameMode == FrameMode::LITTLE_FRAME)
	{
		//小框的绘制临时停止绘制策略
		isSuspended = true;

	}
	//moveMouse(up, is2D);
}


QStringList hn2d3dPixBaseWidget::getDiseaseTypes()
{
	//获取当前病害种类
	QStringList result;
	const QVector<hnDiseaseSetInfo> diseaseSetInfos = this->getDiseaseSetInfos();

	for (auto diseseSetInfo : diseaseSetInfos)
	{
		result.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}

	return result;
}

QVector<hnDiseaseSetInfo> hn2d3dPixBaseWidget::getDiseaseSetInfos()
{
	QVector<hnDiseaseSetInfo> result;

	//获取病害类型
	if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE ||
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
	{
		QPoint bigImageStart = this->singleImagePointToBigImagePoint(m_diseaseStartPoint.pixPoint, m_diseaseStartPoint.pixName);
		//获取第一个点的hnMile
		hnMile mile = this->getHnMileFromPoint(bigImageStart);
		result = hnApp::hnDataManager::getDataManager()->getCurrentProjectRoadDiseases(mile);
	}
	else
	{
		//单三维的就按人工模式的病害类型来
		hnProjectSetInfo setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		result = hnApp::hnDataManager::getDataManager()->getRoadDisease(HnProjectEnums::roadTypeQStringToEnum(setting.strRoadStandard),
			(ROAD_WORK_TYPE)0, (ROAD_SURFACE_TYPE)setting.nRSurfaceType, 0);
	}

	return result;
}

void hn2d3dPixBaseWidget::lineDiseaseAddDisease()
{
	const auto projectType = hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (PROJECT_23D_TYPE == projectType || PROJECT_2D_TYPE == projectType)
	{
		if (!this->isTmpDiseaseRoadTypeValid(0))
		{
			QMessageBox::warning(nullptr, QString::fromLocal8Bit("警告"),
				QString::fromLocal8Bit("所画病害中有不同的路面标准或者路面类型，病害无效，取消绘制"),
				QString::fromLocal8Bit("确定"));
			this->slot_cancelDrawDiseases();
			return;
		}
	}

	//获取病害类型	
	QStringList diseaseTypeList;
	//diseaseTypeList = this->getDiseaseTypes();  

	QList<QPair<QString, QString>> diseaseNameAndKey;
	hnDiseaseSetInfo selectDiseaseSetInfo;
	const auto diseaseSetInfos = this->getDiseaseSetInfos();
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		diseaseNameAndKey.append( qMakePair( QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName), QString(diseseSetInfo.nShortcutKey)));
		diseaseTypeList.append(QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName));
	}
	//弹出添加病害窗口
	addDiseaseDialog dialog(diseaseNameAndKey, false);
	dialog.setWindowTitle(QString::fromLocal8Bit("添加病害"));
	dialog.setDiseaseAttributeEnabled(false);
	QString diseaseTypeName;
	if (dialog.exec() == QDialog::Accepted)
	{
		diseaseTypeName = dialog.getDiseaseTypeName();
	}
	else
	{
		this->slot_cancelDrawDiseases();
		return;
	}

	//计算病害属性
	QString diseaseTableName;

	
	for (auto diseseSetInfo : diseaseSetInfos)
	{
		if (QString::fromLocal8Bit(diseseSetInfo.strDiseaseTypeName) == diseaseTypeName)
		{
			selectDiseaseSetInfo = diseseSetInfo;
			diseaseTableName = QString::fromLocal8Bit(diseseSetInfo.strDBTableName);
			break;
		}
	}
	//需要计算属性
	hnRoadDiseaseInfo  diseaseInfo = this->caculateLineDiseaseInfo(m_tmpPaintLineDiseasePoints, selectDiseaseSetInfo);

	//for (int i = 0; i < 1000; i++)

	diseaseInfo.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
		->getDB()->m_diseaseTable.getMaxID(diseaseTableName.toLocal8Bit().data());
	//线状病害没有深度  深度计算
#if 0
	if (false == diseaseInfo.vec3dRect.empty() && true == m_isOPenDepthCaculate)
	{
		if (false == this->caculateBigFrameDiseaseDepth(diseaseInfo))
		{
			this->slot_cancelDrawDiseases();
			return;
		}
	}
#endif


	auto setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();


	//写入数据库
	if (	hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.writeSingleDatas(setting, diseaseInfo)) 
	{
		emit this->signal_addDisease(diseaseInfo,false);

	}
	//发信号 数据库变化
	
	this->slot_cancelDrawDiseases();

	return;
}

void hn2d3dPixBaseWidget::drawLineDiseases(const vector<hnRoadDiseaseInfo>& diseases, QImage & image)
{
	if (true == diseases.empty())
	{
		return;
	}

	QPainter painter(&image);
	QPen pen;
	pen.setWidth(m_lineWidth);
	pen.setColor(Qt::red);

	painter.setPen(pen);

	QFont font = painter.font();
	font.setBold(true);
	font.setPixelSize(m_fontSize);
	painter.setFont(font);
	for (auto disease : qAsConst(diseases))
	{
		if (3 != disease.nDrawType)
		{
			continue;
		}
		if (true == isSeclectedMergeDisease(disease))
		{
			pen.setColor(Qt::yellow);
			painter.setPen(pen);
		}
		else
		{
			pen.setColor(Qt::red);
			painter.setPen(pen);
		}

		QVector<QPoint> points;
		points = this->createBrokenLinePoints(disease);

		for (int i = 0; i < points.size() - 1; i++)
		{
			QLine line(points.at(i), points.at(i + 1));
			painter.drawLine(line);

			if (0 == i)
			{
				QString diseaseInfo = QString::fromLocal8Bit(disease.strDisName)
					+ "_" + QString::number(disease.nID)+"_"+QString::number(disease.dArea,'f',2);
				painter.drawText(points.at(i), diseaseInfo);
			}
		}
	}
}

void hn2d3dPixBaseWidget::drawTmpLineDiseases(QImage &image)
{
	if (m_tmpPaintLineDiseasePoints.isEmpty())
	{
		return;
	}
	QPainter painter(&image);
	QPen pen;
	pen.setWidth(m_lineWidth);
	pen.setStyle(Qt::DashLine);
	pen.setColor(Qt::red);
	painter.setPen(pen);

	QFont font = painter.font();
	font.setPixelSize(m_fontSize);
	painter.setFont(font);

	for (int i = 0; i < m_tmpPaintLineDiseasePoints.size() - 1; i++)
	{
		QPoint begin = this->singleImagePointToBigImagePoint(
			m_tmpPaintLineDiseasePoints.at(i).pixPoint, m_tmpPaintLineDiseasePoints.at(i).pixName);
		QPoint end = this->singleImagePointToBigImagePoint(
			m_tmpPaintLineDiseasePoints.at(i + 1).pixPoint, m_tmpPaintLineDiseasePoints.at(i + 1).pixName);
		QLine line(begin, end);
		painter.drawLine(line);
	}
	 
}

//2025.11.3 新增绘制最后一个点与鼠标连线（虚线）
void hn2d3dPixBaseWidget::drawTempDashLine(QImage &image)
{
	QPainter painter(&image);
	QPen pen;
	pen.setWidth(15);
	pen.setStyle(Qt::DashLine);
	pen.setColor(Qt::yellow);
	painter.setPen(pen);

	QFont font = painter.font();
	font.setPixelSize(m_fontSize);
	painter.setFont(font);

	QPoint startPoint = singleImagePointToBigImagePoint(m_tempPoints[0].pixPoint, m_tempPoints[0].pixName);
	QPoint endPoint = singleImagePointToBigImagePoint(m_tempPoints[1].pixPoint, m_tempPoints[1].pixName);
	QLine line(startPoint, endPoint);
	painter.drawLine(line);
}


double hn2d3dPixBaseWidget::calculateLineDiseaseCenterMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;

	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = 0.5*(mileMap.first() + mileMap.last());

	return result;
}

double hn2d3dPixBaseWidget::calculateLineDiseaseBeginMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;
	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = mileMap.first();

	return result;
}

double hn2d3dPixBaseWidget::calculateLineDiseaseEndMile(QVector<pixImagePoint> lineDiseasePoints)
{
	double result = 0.0;
	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}

	auto mileMap = this->createLineDiseaseMileMap(lineDiseasePoints);

	result = mileMap.last();

	return result;
}

QMap<int, double> hn2d3dPixBaseWidget::createLineDiseaseMileMap(const QVector<pixImagePoint>& lineDiseasePoints)
{
	QMap<int, double> result;

	if (true == lineDiseasePoints.isEmpty())
	{
		return result;
	}
	if (nullptr == hnApp::hnDataManager::getDataManager()->getCurrentProject())
	{
		return result;
	}

	int count = 0;

	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		const QPoint bigImagePoint = this->singleImagePointToBigImagePoint(lineDiseasePoint.pixPoint, lineDiseasePoint.pixName);
		const double mile = this->caculateEncoderMileByBigImagePoint(bigImagePoint);
		result.insert(count, mile);
		count++;
	}

	return result;
}

void hn2d3dPixBaseWidget::addLineDisease(const QPoint & widgetPoint)
{
	this->m_isDrawingDisease = true;
	this->m_isAllowDrawPix = false;

	QString pixName;
	QPoint singleImagePoint = this->screenToSingleImagePoint(widgetPoint, pixName);

	//三维视图画点越界（相对于二维视图）的处理
	QString className = this->metaObject()->className();
	if (PROJECT_23D_TYPE == hnDataManager::getDataManager()->getCurrentProject()->getProjectType() && true == className.contains("3d"))
	{
		int x = singleImagePoint.x();
		this->autoCorrectXIn3dView(x);
		singleImagePoint.setX(x);
	}

	pixImagePoint pixPoint;
	pixPoint.pixName = pixName;
	pixPoint.pixPoint = singleImagePoint;

	if (true == m_tmpLineDiseasePoints.empty())
	{
		//记录开始点
		m_diseaseStartPoint.pixPoint = this->screenToSingleImagePoint(widgetPoint, m_diseaseStartPoint.pixName);
	}
	m_tmpLineDiseasePoints.append(pixPoint);
	m_tmpLastPaintLineDiseasePoints.clear();
}

void hn2d3dPixBaseWidget::commonDeleteDisease(const QPoint & mousePoint, hnFrameMode::FrameMode frameMode)
{
	for (auto disease : this->m_currentWidgetDiseases)
	{

		if (this->isInDisease(mousePoint, disease, frameMode))
		{
			//选中病害插入要删除的病害
			m_seclectedDiseases.clear();
			m_seclectedDiseases.append(disease);

			 
				//从数据库中删除该病害
				hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable.deleteDisease(disease);
				emit this->signal_deleteDisease(disease);
				m_isAllowDrawPix = true;
				//清空选中病害
				m_seclectedDiseases.clear();
				this->update();
				return; 
		}
	}
}

void hn2d3dPixBaseWidget::commonEditDisease(const QPoint & mousePoint, hnFrameMode::FrameMode frameMode)
{
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(mousePoint, disease, frameMode))
		{
			this->editDisease(disease, mousePoint);
		}
	}
}

void hn2d3dPixBaseWidget::mergeLineDisease(const QPoint & widgetPoint)
{
	//获取选中的病害，添加到数组中
	for (auto disease : this->m_currentWidgetDiseases)
	{
		if (this->isInDisease(widgetPoint, disease, hnFrameMode::DESIGN_LINE))
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
	if (2 == m_seclectedDiseases.size())
	{
		//新病害
		auto newDisease = m_seclectedDiseases.at(0);

		//新病害有些属性继承了合并病害的第一个，有很多属性要重新计算
		hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

		//清空坐标信息
		newDisease.vec3dRect.clear();
		newDisease.vec2dRect.clear();

		//计算2d的坐标信息
		if (nullptr != hnApp::hnDataManager::getDataManager()->getCurrentProject()->get2DProject())
		{
			auto vec2dRect0 = m_seclectedDiseases.at(0).vec2dRect;
			auto vec2dRect1 = m_seclectedDiseases.at(1).vec2dRect;
			vec2dRect0.insert(vec2dRect0.end(), vec2dRect1.begin(), vec2dRect1.end());
			newDisease.vec2dRect = vec2dRect0;
		}

		//计算3d坐标信息
		if (nullptr != hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
		{
			auto vec3dRect0 = m_seclectedDiseases.at(0).vec3dRect;
			auto vec3dRect1 = m_seclectedDiseases.at(1).vec3dRect;
			vec3dRect0.insert(vec3dRect0.end(), vec3dRect1.begin(), vec3dRect1.end());
			newDisease.vec3dRect = vec3dRect0;
			//深度计算，如需
			//...

		}

		// 线状病害长度
		newDisease.dLength = m_seclectedDiseases.at(0).dLength + m_seclectedDiseases.at(1).dLength;

		// 获取病害的数组
		auto lineDiseaseBigImagePoints = this->createBrokenLinePoints(newDisease);
		QVector<pixImagePoint> pixImagePoints;
		for (auto lineDiseaseBigImagePoint : qAsConst(lineDiseaseBigImagePoints))
		{
			pixImagePoint point;
			point.pixPoint = this->bigImagePointToSingleImagePoint(lineDiseaseBigImagePoint, &point.pixName);
			pixImagePoints.append(point);
		}

		//中心里程
		newDisease.dMileage = this->calculateLineDiseaseCenterMile(pixImagePoints);
		//开始里程
		newDisease.dDmiStart = this->calculateLineDiseaseBeginMile(pixImagePoints);
		//结束里程
		newDisease.dDmiEnd = this->calculateLineDiseaseEndMile(pixImagePoints);

		//计算病害的计算面积 
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(  newDisease);

		//重新设置ID
		newDisease.nID = hnApp::hnDataManager::getDataManager()->getCurrentProject()
			->getDB()->m_diseaseTable.getMaxID(newDisease.strDiseaseTableName);

		//新病害写入数据库
		auto setting = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	if(	hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.writeSingleDatas(setting, newDisease))

		//发信号 数据库变化
		emit this->signal_addDisease(newDisease,false);

		//删除第一个病害
		auto firstDisease = m_seclectedDiseases.at(0);
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.deleteDisease(firstDisease);
		//发信号 数据库变化
		emit this->signal_deleteDisease(firstDisease);

		//删除第二个病害
		auto secondDisease = m_seclectedDiseases.at(1);
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.deleteDisease(secondDisease);
		//发信号 数据库变化
		emit this->signal_deleteDisease(secondDisease);

		this->update();

		//清空选中的病害数组
		m_seclectedDiseases.clear();
	}
}

bool hn2d3dPixBaseWidget::isNearbyLineDisease(const QPoint & bigImagePoint, const hnRoadDiseaseInfo & disease)
{
	const  int minDistance = 30;
	hnRoadDiseaseInfo info = disease;
	auto points = createBrokenLinePoints(info);
	if (true == points.empty())
	{
		return false;
	}
	for (int i = 0; i < points.size() - 1; i++)
	{
		const QLineF line(points.at(i), points.at(i + 1));
		const double distance = distanceFromPointToLine(bigImagePoint, line);
		if (distance <= minDistance)
		{
			return true;
		}
	}
	return false;
}



double hn2d3dPixBaseWidget::caculateLineDiseaseLenth(QVector<pixImagePoint> lineDiseasePoints, WidgetType widgetType)
{
	double result = 0.0;
	if (true == lineDiseasePoints.empty())
	{
		return result;
	}

	QVector<QPoint> bigImagePoints;

	//转成大image的point
	for (auto lineDiseasePoint : qAsConst(lineDiseasePoints))
	{
		QPoint bigImagePoint = this->singleImagePointToBigImagePoint(lineDiseasePoint.pixPoint, lineDiseasePoint.pixName);
		bigImagePoints.push_back(bigImagePoint);
	}
	// 横向比例
	double widthScale, heightScale;
	if (WIDGET_2D == widgetType)
	{
		widthScale = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;
		heightScale = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;
	}
	else if (WIDGET_3D == widgetType)
	{
		widthScale = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
		heightScale = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageHeightScale();
	}
	else
	{
		return result;
	}


	//算每个线段的长度，然后加起来
	for (int i = 0; i < bigImagePoints.size() - 1; i++)
	{
		QLineF line(bigImagePoints.at(i), bigImagePoints.at(i + 1));

		double width = line.dx() * widthScale;
		double height = line.dy() * heightScale;
		double lenth = qSqrt(width * width + height*height);

		result += lenth;
	}

	return result;
}

void hn2d3dPixBaseWidget::autoCorrectXIn3dView(int & x)
{
	const double roadWidth2d = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadWidth;
	const double roadWidth3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getRoadWidth();
	const double scale3d = hnDataManager::getDataManager()->getCurrentProject()->get3DProject()->getImageWidthScale();
	const int pixel2d3dBoarderWidth = 0.5*((roadWidth3d - roadWidth2d) / scale3d);
	const int leftBoarder = pixel2d3dBoarderWidth;
	const int rightBoarder = m_pixWidth - pixel2d3dBoarderWidth;

	if (x < leftBoarder)
	{
		x = leftBoarder;
		return;
	}
	if (x > rightBoarder)
	{
		x = rightBoarder;
		return;
	}
}



void hn2d3dPixBaseWidget::moveMouse(bool up, bool is2D)
{

	//if (this->m_workMode == WorkMode::ADD_MODE&&this->m_isDrawingDisease)
	//{
	//	// 修改鼠标位置
	//	int moveImagePixel = this->m_pixHeight / 2;
	//	int moveScreenPixel = round((moveImagePixel*this->width()*1.0) / this->m_pixWidth);
	//	QPointF dst;
	//	if (up)
	//	{
	//		dst = currentMousePos;
	//		dst.ry() += moveScreenPixel;
	//	}
	//	else
	//	{
	//		dst = currentMousePos;
	//		dst.ry() -= moveScreenPixel;
	//	}

	//	// 计算窗口在屏幕的坐标范围
	//	QPointF w_topLeftAtScreen = this->mapToGlobal(QPoint(0, 0));
	//	QPointF w_bottomRightAtScreen = this->mapToGlobal(QPoint(this->width(), this->height()));
	//	if (dst.rx() > w_topLeftAtScreen.rx() &&
	//		dst.rx() < w_bottomRightAtScreen.rx() &&
	//		dst.ry() > w_topLeftAtScreen.ry() &&
	//		dst.ry() < w_bottomRightAtScreen.ry())
	//	{
	//		QCursor::setPos(dst.toPoint());				// 不超过窗口区域，就进行移动
	//	}

	//	//2025.11.7 延时刷新画病害（等待底图先更新）
	//	QTimer::singleShot(15, [this]() {
	//		if (this->addLineDiseType && this->m_isDrawingDisease && !this->m_isEndAddPoint)
	//		{
	//			this->m_isAllowDrawPix = false;
	//			this->update();
	//		}
	//	});

	//	//QCursor::setPos(dst.toPoint());
	//}

	//return;

	
}

QPoint hn2d3dPixBaseWidget::pixImagePointToBigImagePoint(const pixImagePoint & point)
{
	QPoint result;

	result = this->singleImagePointToBigImagePoint(point.pixPoint, point.pixName);

	return result;
}

void hn2d3dPixBaseWidget::setLittleDiseaseSize(const QVector<QRect>& diseaseRects, hnRoadDiseaseInfo& disease)
{ 
	QVector<QRect>rects = QVector<QRect>::fromList(diseaseRects.toList());
	QRect boundingRect;
	if (!rects.isEmpty())
	{
		int rectHeight = qAbs(rects[0].height());
		int rectWidth = qAbs(rects[0].width());
		for (const QRect& rect : rects)
		{
			boundingRect = boundingRect.united(rect);
		}

		disease.dLength = std::round(static_cast<double>(boundingRect.height()) / rectHeight)*0.1;
		disease.dWidth = std::round(static_cast<double>(boundingRect.width()) / rectWidth)*0.1;
	} 
}

void hn2d3dPixBaseWidget::reCalculateDiseaseSizeAndSave(hnCommon::hnRoadDiseaseInfo & disease, bool save)
{
	//自动化模式病害计算最大外接矩形尺寸，并且为了适配之前版本 尺寸为0的 情况在此处更新病害
	int drawType =  hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	
	if (disease.nDrawType ==3 ||disease.nDrawType ==2)  //设计模式的不参与计算
	{
		return;
	}
	if (drawType != disease.nDrawType)
	{
		if (save)
		{
			disease.nDrawType = drawType;
			hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
			//写入数据库
			if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.updaetSingleDataInfo(disease));

			//发信号 数据库变化
			emit this->signal_addDisease(disease,true);
		}
	}
	
}

void hn2d3dPixBaseWidget::reCalculateOldDiseaseSizeAndSave(const QVector<QRect>& diseaseRects,hnCommon::hnRoadDiseaseInfo& disease)
{
	
	{
		setLittleDiseaseSize(diseaseRects,disease);
		hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(disease);
		 
			//写入数据库
			if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->m_diseaseTable.updaetSingleDataInfo(disease));

			//发信号 数据库变化
			emit this->signal_addDisease(disease,true);
	}
}

void hn2d3dPixBaseWidget::CalculateDiseaseSize(const QVector<QRect>&diseaseRects, hnCommon::hnRoadDiseaseInfo& disease)
{
	if (disease.dLength == 0 || disease.dArea == 0)
	{ 
		setLittleDiseaseSize(diseaseRects, disease); 
	}
}

QVector<hnCommon::hnRoadDiseaseInfo> hn2d3dPixBaseWidget::getAllRoadDiseaseTemp()
{
	 //获取所有病害
	 if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	 {
		 return  QVector<hnRoadDiseaseInfo>();
	 }
	 QVector<hnRoadDiseaseInfo> allDiseaseInfos;

	 hnPro::hnProject* project = hnApp::hnDataManager::getDataManager()->getCurrentProject();
	 QString standard = HnProjectEnums::roadTypeEnumToQString(project->getBaseStandard());
	 if (PROJECT_TYPE::PROJECT_23D_TYPE == hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType() ||
		 PROJECT_TYPE::PROJECT_2D_TYPE == hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType())
	 {

		 // 二三维病害

		 QVector<hnMile> miles = project->getCurrentMileVector();
		 auto setinfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
		 if (setinfo.nDrawType == 2)
		 {
			 hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->
				 m_diseaseTable.readDesignDiseases(standard, 0, project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
		 }
		 else
		 {
			 hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable.readRoadDiseaseData(setinfo, miles, allDiseaseInfos, project->getCurrentMarkVector(), project->getRoadSpace());
		 }
	 }
	 else
	 {
		 std::vector<hnRoadDiseaseInfo> diseases3d;
		 // 纯三维病害
		 const double projectBeginEncoderMile = 0;
		 const double projectEndEncoderMile = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dEndEnclMile;
		 auto xxx = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

		 if (hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType == 2)
		 {
			 hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->
				 m_diseaseTable.readDesignDiseases(standard, 0, project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
		 }
		 else
		 {
			 hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()
				 ->m_diseaseTable.read3dRoadDiseaseData(standard, projectBeginEncoderMile, projectEndEncoderMile, diseases3d);
			 allDiseaseInfos = QVector<hnRoadDiseaseInfo>::fromStdVector(diseases3d);
		 }
	 }

	 return allDiseaseInfos;
}

