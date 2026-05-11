#include "hnDiseaseListWidget.h"
#include <QGridLayout>
#include <QHeaderView>
#include "../hnQtCommon/hnMile.h"
#include <QVector>
#include <QMenu>
#include <QAction>
#include <functional>
#include <QEvent>
#include <QTimer>
#include <QScrollBar>
hnDiseaseListWidget::hnDiseaseListWidget(QWidget *parent)
	:  QWidget(parent),
      m_model(new QStandardItemModel(this)),
      m_view(new customTableView(this)),
      filterComboBox(nullptr),
      sortDiseaseTypeModel(new QSortFilterProxyModel(this))

	QHeaderView *verticalHeader = m_view->verticalHeader();
	verticalHeader->setVisible(true);
	verticalHeader->setDefaultSectionSize(35);				// 设置病害列表行高固定
	//verticalHeader->setSectionResizeMode(QHeaderView::Stretch);

	//设置表格视图选择行为  选择为整行
	this->m_view->setSelectionBehavior(QAbstractItemView::SelectRows);

	//设置表格视图选择模式为单选模式
	this->m_view->setSelectionMode(QAbstractItemView::SingleSelection);

	//禁止编辑功能
	this->m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	
	
	this->m_model->clear();
	this->initTableHeader(m_model);
	//设置源模型 
this->sortDiseaseTypeModel->setSourceModel(m_model);
this->sortDiseaseTypeModel->setDynamicSortFilter(true);
this->sortDiseaseTypeModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
this->sortDiseaseTypeModel->setFilterKeyColumn(m_diseaseTypeColumn);

this->m_view->setModel(this->sortDiseaseTypeModel);

	//QGridLayout *mainGridLayout = new QGridLayout;
	//this->setLayout(mainGridLayout);
	//mainGridLayout->addWidget(m_view);

	QVBoxLayout * mainGridLayout = new QVBoxLayout;
	  filterComboBox =  new QComboBox(this);
	  populateComboBox();
	filterComboBox->setEditable(false);
	mainGridLayout->addWidget(filterComboBox);
	 this->setLayout(mainGridLayout);
	mainGridLayout->addWidget(m_view);


	 


	//信号槽连接 双击表格，进行处理
	connect(this->m_view, &QTableView::doubleClicked,this, &hnDiseaseListWidget::slot_itemDoubleClicked);
	QHeaderView* header =  this->m_view->horizontalHeader();
	
	header->setSectionsClickable(true);
	//proxyModel = new QSortFilterProxyModel(this);
	//proxyModel->setSourceModel(m_model);
	 
	connect(header, &QHeaderView::sectionClicked, this, &hnDiseaseListWidget::on_section_clicked);
	connect(header, &QHeaderView::sectionDoubleClicked, this,&hnDiseaseListWidget::on_section_doubleClicked);
	connect(m_view, &customTableView::customContextMenuRequested, this, &hnDiseaseListWidget::slot_MenuClicked);
	connect(filterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &hnDiseaseListWidget::slot_selectDiseaseTypeIndexChanged);
}

hnDiseaseListWidget::~hnDiseaseListWidget()
{
}

void hnDiseaseListWidget::updateAllDiseases()
{
	this->m_model->clear();												//清理表			
	this->initTableHeader(this->m_model);								//初始化表头
	QVector<hnRoadDiseaseInfo> allDiseaseInfos = this->getAllDisease();	//获取所有病害
	this->addDiseaseToTable(allDiseaseInfos, this->m_model);			//遍历病害，添加到表上

	 if (sortDiseaseTypeModel)
    {
        sortDiseaseTypeModel->setFilterKeyColumn(m_diseaseTypeColumn);
        sortDiseaseTypeModel->invalidate();
    }
}

void hnDiseaseListWidget::addDisease(const hnRoadDiseaseInfo & disease,bool modify)
{
	 if (!m_model || !sortDiseaseTypeModel)
        return;

    if (modify)
    {
        deleteDisease(disease);
    }

    modelAddDisease(*m_model, disease);

    sortDiseaseTypeModel->invalidate();

    slot_selectDisease(disease);
	 
}

void hnDiseaseListWidget::deleteDisease(const hnRoadDiseaseInfo & disease)
{
	modelDeleteDisease(*m_model, disease);

	 
}

void hnDiseaseListWidget::editDisease(const hnRoadDiseaseInfo & disease)
{
	//先删除
	modelDeleteDisease(*m_model, disease);
	//再添加
	modelAddDisease(*m_model, disease);
	//设置模型
	 
}

void hnDiseaseListWidget::seclectLastRowDisease()
{
	QModelIndex currentIdx = m_view->currentIndex();
	int currentRow = currentIdx.row();
	if (currentRow < 0)
	{
		currentRow = 1;
	}
	m_view->selectRow(currentRow - 1);
	currentIdx = m_view->currentIndex();
	slot_itemDoubleClicked(currentIdx);
}

void hnDiseaseListWidget::seclectNextRowDisease()
{
	QModelIndex currentIdx = m_view->currentIndex();
	int currentRow = currentIdx.row();
	m_view->selectRow(currentRow + 1);
	currentIdx = m_view->currentIndex();
	slot_itemDoubleClicked(currentIdx);
}



void hnDiseaseListWidget::modelDeleteDisease(QStandardItemModel & model, const hnRoadDiseaseInfo & disease)
{
	QModelIndexList deleteList;
	//这是一个过滤模型的类，我们使用这个类，快速的找到该病害所在的行，然后删掉它
	QSortFilterProxyModel sortModel;
	//设置源模型
	sortModel.setSourceModel(&model);
	//设置想要过滤的列，也就是id的那一列，就是第0列
	sortModel.setFilterKeyColumn(m_idColumn);
	//设置过滤条件为病害的ID
	sortModel.setFilterRegExp(QString::number(disease.nID));

	//遍历过滤后的model，然后找到病害表名和传入病害的相同的那一个
	const int rowCount = sortModel.rowCount();
	const int tableNameColumnCount = m_tableNameColumn;
	for (int row = 0; row < rowCount; row++)
	{
		//获取病害表名列的索引
		QModelIndex idx = sortModel.index(row, tableNameColumnCount);
		//获取该索引的表名
		QString tableName = sortModel.data(idx).toString();
		//与病害表名比较
		if (tableName == QString::fromLocal8Bit(disease.strDiseaseTableName))
		{
			//获取原模型的索引
			QModelIndex srcIdx = sortModel.mapToSource(idx);
			
			//获取原模型的行
			int srcRow = srcIdx.row();
			//删除原模型的行
		//	model.removeRow(srcRow);
			deleteList.push_back(srcIdx);
			//退出循环
			break;
		}	
	}
	deleteDiseases(deleteList);
}

QStandardItem* hnDiseaseListWidget::createNumericItem(const QString &text)
{
	QStandardItem * item = new QStandardItem;
	bool ok;
	int number = text.toInt(&ok);
	if (!ok)
	{
		item->setData(QVariant(text.toDouble()), Qt::EditRole);
	}
	if (ok)
	{
		item->setData(QVariant(number), Qt::EditRole);
	}
	return item;
}

void hnDiseaseListWidget::slot_itemDoubleClicked(const QModelIndex & index)
{
	//获取所在行数
	int row = index.row();

	//获取病害中心里程所在的列
	QModelIndex regionIdx = this->sortDiseaseTypeModel->index(row, m_centerMileColumn);
	QModelIndex idIdx = this->sortDiseaseTypeModel->index(row, m_idColumn);
	//获取病害的编码器里程
	double region = this->sortDiseaseTypeModel->data(regionIdx).toDouble();
	int id = this->sortDiseaseTypeModel->data(idIdx).toInt();

	auto projectType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getProjectType();
	if (PROJECT_23D_TYPE == projectType || PROJECT_2D_TYPE == projectType)
	{
		//路面2d图像视图跳转
		this->road2dDiseaseJump(region,id);
	}
	else
	{
		this->road3dDiseaseJump(region,id);
	}
}

void hnDiseaseListWidget::on_section_clicked(int logicalIndex)
{
	m_model->sort(logicalIndex); 
}

void hnDiseaseListWidget::on_section_doubleClicked(int logicalIndex)
{
	m_model->sort(logicalIndex,Qt::DescendingOrder);

}

void hnDiseaseListWidget::slot_MenuClicked(const QPoint &pos)
{
 
	QModelIndex index = m_view->indexAt(pos);
	if (!index.isValid())
	{
		return;
	}
	QMenu menu(m_view);

	QAction *deleteAction = menu.addAction(QStringLiteral("删除"));

	connect(deleteAction, &QAction::triggered, this, [&]() {
		QItemSelectionModel *selectModel = m_view->selectionModel();
		QModelIndexList selectedIndexes = selectModel->selectedIndexes();
		//删除病害
		deleteDiseases(selectedIndexes);
		 
	} );
	//鼠标位置显示
	menu.exec(m_view->viewport()->mapToGlobal(pos));
}

void hnDiseaseListWidget::slot_selectDiseaseTypeIndexChanged(int index)
{ 
	QString regExpStr;
	switch (index)
	{

	case  0 :
		sortDiseaseTypeModel->setFilterRegExp(QRegExp()); 
		break;
	case 1:
		regExpStr = "^0$";
		break;
	case 2:
		regExpStr = "^1$";
		break;
	case 3:
		regExpStr = "^2$";
		break;
	default:
		break;
	}
	 if (!regExpStr.isEmpty())
	 {
		 QRegExp regExp(regExpStr, Qt::CaseInsensitive, QRegExp::RegExp);//正则
		 sortDiseaseTypeModel->setFilterRegExp(regExp);
	 }
	 else
	 {
		 sortDiseaseTypeModel->setFilterRegExp(QRegExp());
	 }
	 sortDiseaseTypeModel->invalidate();

}

void hnDiseaseListWidget::deleteDiseases(QModelIndexList selectedIndexes)
{


	QSet<int> rowsToRemove;;
	QList<hnRoadDiseaseInfo> rowDataList;//存储每一行的Id
	for (const QModelIndex &index:selectedIndexes)
	{
		int row = index.row();
		rowsToRemove.insert(row);

		QModelIndex firstColumnIndex = sortDiseaseTypeModel->index(row, 0);
		 
		if (firstColumnIndex.isValid())
		{
		QVariant data = sortDiseaseTypeModel->data(firstColumnIndex, Qt::UserRole);
		 if (data.canConvert<hnRoadDiseaseInfo>())
		 {
			 hnRoadDiseaseInfo disease = data.value<hnRoadDiseaseInfo>();
			 bool exists = false;
			 for (const hnRoadDiseaseInfo &existing : rowDataList)
			 {
				 if (existing.nID == disease.nID && strcmp(  existing.strDiseaseTableName,disease.strDiseaseTableName) ==0)
				 {
					 exists = true;
					 break;
				 }

			 }
			 if (!exists)
			 {
				 rowDataList.append(disease);
				 //从数据库中删除该病害
				 hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable.deleteDisease(disease);
				 emit signal_deleteDisease(disease);
				 emit signal_updateView();
			 }
		 }
			 
		}
	}

	QList<int >sortedRows = rowsToRemove.toList();
	std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());
	int oldRowCount = m_model->rowCount();
	int row = sortedRows.first();
	if (row < 0 || row >= oldRowCount)
	{
		return;
	}
	for (int row:sortedRows)
	{
		m_model->removeRow(row);
	}

	int newRowCount = m_model->rowCount();
	
	if (newRowCount <=0 )
	{
		return;
	}

	int nextRow = row;
	if (nextRow >= newRowCount)
	{
		nextRow = newRowCount - 1;
	}

	QModelIndex nextIndex = m_model->index(nextRow, 0);
	if (!nextIndex.isValid())
	{
		return;
	}
	//m_view->setCurrentIndex(nextIndex);
	//m_view->selectionModel()->select(nextIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

	slot_itemDoubleClicked(nextIndex);
}

void hnDiseaseListWidget::slot_selectDisease(const hnRoadDiseaseInfo& disease)
{
	//选中病害，获取病害位置
	QModelIndex selectIndex = 	getUserSelectIndex(sortDiseaseTypeModel, disease);
	MoveScrollBar(m_view, selectIndex);

}

void hnDiseaseListWidget::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Up)
	{
		this->seclectLastRowDisease();
	}
	else if(event->key() == Qt::Key_Down)
	{
		this->seclectNextRowDisease();
	}

	if (event->key() == Qt::Key_Delete)
	{
		//获取选中的行
		QItemSelectionModel *selectModel = m_view->selectionModel();
		QModelIndexList selectedIndexes = selectModel->selectedIndexes();
		//删除病害
		deleteDiseases(selectedIndexes);
	}

	qDebug() << "hnDiseaseListWidget::keyPressEvent";
	QWidget::keyPressEvent(event);
}

void hnDiseaseListWidget::road2dDiseaseJump(const double encoderMile, const int diseaseID)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	auto projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();
	double roadLenth = projectSetInfo.dRoadLength;
	if (roadLenth == 0)
	{
		return;
	}
	int frameIdx = encoderMile / roadLenth;

	//发信号出去，病害角标
	emit this->signal_road2dFrameIdxChanged(frameIdx);
	emit this->signal_setDiseaseIsChecked(diseaseID);
}

void hnDiseaseListWidget::road3dDiseaseJump(const double encoderMile, const int diseaseID)
{
	const int roadHeight = 8;

	const int frameIdx = encoderMile / roadHeight;

	emit this->signal_road3dFrameIdxChanged(frameIdx);
	emit this->signal_setDiseaseIsChecked(diseaseID);
}

void hnDiseaseListWidget::streetJump(const double encoderMile)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}

	double streetDistance = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getStreetSpace();

	int frameIdx = encoderMile / streetDistance;

	//发信号出去，病害角标
	emit this->signal_streetFrameIdxChanged(frameIdx);
}

void hnDiseaseListWidget::modelAddDisease(QStandardItemModel & model, const hnRoadDiseaseInfo & disease)
{
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return;
	}
	//计算信息后的病害
	hnRoadDiseaseInfo newDisease = disease;

	QList<QStandardItem*> rowStandardItems;
	QString disName = QString::fromLocal8Bit(disease.strDisName);
	QString disMark = QString::fromLocal8Bit(disease.strRemark);
	if (!disName.contains(QStringLiteral("修补")))
	{
		disName = disName.split(".").first();
	}

	QStandardItem * standId = createNumericItem(QString::number(newDisease.nID));
	standId->setData(QVariant::fromValue(disease), Qt::UserRole);
	//病害id
	rowStandardItems.append(standId);

	//病害名字
	rowStandardItems.append(new QStandardItem(disName));
	//病害等级
	rowStandardItems.append(new QStandardItem(this->diseaseLevelIntToQString(newDisease.nLevel)));

	 
	rowStandardItems.append(new QStandardItem(disMark));
	//桩号
	int mile = qRound(hnApp::hnDataManager::getDataManager()->getCurrentProject()->enclToTrueMile(newDisease.dMileage));


	//rowStandardItems.append(new QStandardItem(QString::number(mile)));
	rowStandardItems.append(createNumericItem(QString::number(mile)));

	//病害中心里程
	//	rowStandardItems.append(new QStandardItem(QString::number(newDisease.dMileage, 'f', 4)));
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dMileage, 'f', 4)));



	//计算面积
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dArea, 'f', 4)));

	//病害长度 
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dLength, 'f', 4)));

	//病害宽度
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dWidth, 'f', 4)));
	//计算长度
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dRealLen, 'f', 4)));
	//计算宽度
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dReaWidth, 'f', 4)));

	//材质
	int type = newDisease.nRSurfaceType;
	QString strType;
	if (type == 0) strType = QString::fromLocal8Bit("沥青");
	else if (type == 1) strType = QString::fromLocal8Bit("水泥");
	else if (type == 2) strType = QString::fromLocal8Bit("砂石");
	rowStandardItems.append(new QStandardItem(strType));
	rowStandardItems.append(new QStandardItem(QString::number(newDisease.diseaseWeight, 'f', 2)));
	//深度
	rowStandardItems.append(createNumericItem(QString::number(newDisease.dDepth * 1000, 'f', 2)));
	//表名
	rowStandardItems.append(new QStandardItem(QString::fromLocal8Bit(newDisease.strDiseaseTableName)));
	 
	//病害类型
	rowStandardItems.append(new QStandardItem( QString::number(  (newDisease.ndiseaseType))));

	//model.appendRow(rowStandardItems);
	 
	
	insertAndSelectRow(m_model, m_view, rowStandardItems, 5);
}
void hnDiseaseListWidget::initTableHeader(QStandardItemModel *model)
{
	const QString idHeader = QString::fromLocal8Bit("ID");
	const QString centerMileHeader = QString::fromLocal8Bit("里程");
	const QString tableNameHeader = QString::fromLocal8Bit("表名");
	const QString disesaseTypeHeader = QString::fromLocal8Bit("病害类型");

	QStringList headers;


		headers << idHeader
			<< QString::fromLocal8Bit("名称") 
			<< QString::fromLocal8Bit("等级")
			<< QString::fromLocal8Bit("备注")
			<< QString::fromLocal8Bit("桩号")
			<< centerMileHeader
			<< QString::fromLocal8Bit("计算面积") 
			<< QString::fromLocal8Bit("长度")
			<< QString::fromLocal8Bit("宽度")
			<< QString::fromLocal8Bit("计算长度")
			<< QString::fromLocal8Bit("计算宽度")
			<< QString::fromLocal8Bit("路面材质")
			<< QString::fromLocal8Bit("权重")
			<< QString::fromLocal8Bit("深度(mm)")
			<< tableNameHeader
			<< disesaseTypeHeader;
	
	
	
	for (int i = 0; i < headers.size(); i++)
	{
		if (idHeader == headers.at(i))
		{
			m_idColumn = i;
		}
		if (disesaseTypeHeader == headers.at(i))
		{
			m_diseaseTypeColumn = i;
		}

		if (centerMileHeader == headers.at(i))
		{
			m_centerMileColumn = i;
		}

		if (tableNameHeader == headers.at(i))
		{
			m_tableNameColumn = i;
		}
	}

	model->setHorizontalHeaderLabels(headers);

 
}

QVector<hnRoadDiseaseInfo> hnDiseaseListWidget::getAllDisease()
{
	//获取所有病害
	if (!hnApp::hnDataManager::getDataManager()->isOpenProject())
	{
		return QVector<hnRoadDiseaseInfo>();
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
				m_diseaseTable.readDesignDiseases(standard,0, project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
		}
		else
		{
			hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable.readRoadDiseaseData(setinfo,miles, allDiseaseInfos, project->getCurrentMarkVector(), project->getRoadSpace());
		}
		//景观病害
		QVector<hnRoadDiseaseInfo> streetDiseases;
		hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()->m_diseaseTable.
			readStreetData(standard,miles, streetDiseases, project->getCurProSetInfo().nLineType, project->getRoadSpace());

		allDiseaseInfos += streetDiseases;
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
				m_diseaseTable.readDesignDiseases(standard,0, project->getCurProSetInfo().dEndEnclMile, allDiseaseInfos);
		}
		else
		{
			hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurDB()
				->m_diseaseTable.read3dRoadDiseaseData(standard,projectBeginEncoderMile, projectEndEncoderMile, diseases3d);
			allDiseaseInfos = QVector<hnRoadDiseaseInfo>::fromStdVector(diseases3d);
		} 
	}

	//返回的病害根据里程排序
	if (project->getCurProSetInfo().nLineType==1)
	{
		std::sort(allDiseaseInfos.begin(), allDiseaseInfos.end());
	}
	else
	{
		std::sort(allDiseaseInfos.begin(), allDiseaseInfos.end(), [](const hnRoadDiseaseInfo&a, const hnRoadDiseaseInfo&b)
		{
			return a.dMileage > b.dMileage;
		}
		);
	}

	return allDiseaseInfos;
}

void hnDiseaseListWidget::addDiseaseToTable(QVector<hnRoadDiseaseInfo> diseases, QStandardItemModel * model)
{
	for (auto diseaseInfo : diseases)
	{ 
		//添加一个病害到model中
		modelAddDisease(*model, diseaseInfo);
	}
}

QString hnDiseaseListWidget::diseaseLevelIntToQString(int intLevel)
{

	QString qstringLevel;
	//0 - 无；1 - 轻；2 - 中； - 3重
	switch (intLevel)
	{
	case 0:
		qstringLevel = QString::fromLocal8Bit("无");
		break;
	case 1:
		qstringLevel = QString::fromLocal8Bit("轻");
		break;
	case 2:
		qstringLevel = QString::fromLocal8Bit("中");
		break;
	case 3:
		qstringLevel = QString::fromLocal8Bit("重");
		break;
	default:
		qstringLevel = QString::fromLocal8Bit("无");
		break;
	}

	return qstringLevel;
}

void hnDiseaseListWidget::populateComboBox()
{
	filterComboBox->clear();
	filterComboBox->addItem(QStringLiteral("全部病害"));
	filterComboBox->addItem(QStringLiteral("路面病害"));
	filterComboBox->addItem(QStringLiteral("沿线设施"));
	filterComboBox->addItem(QStringLiteral("路基损坏"));
	filterComboBox->setCurrentIndex(0);

}

void hnDiseaseListWidget::insertAndSelectRow(QStandardItemModel* model, QTableView *view, const QList<QStandardItem*>&newRow, int mileageColIndex)
{
	if (!model || !view || newRow.isEmpty())return;

	double newMileage = newRow[mileageColIndex]->data(Qt::DisplayRole).toDouble();

	int low = 0;
	int high = model->rowCount();

	while (low< high)
	{
		int mid = low + (high - low) / 2;

		//获取中间里程
		QStandardItem * midItem = model->item(mid, mileageColIndex);
		double midMile = midItem->data(Qt::DisplayRole).toDouble();

		bool moveRight = false;
		
		{
			if (midMile< newMileage)
			{
				moveRight = true;
			}
		}
		/*else if (direction==-1)
		{
			if (midMile> newMileage)
			{
				moveRight = true;
			}
		}*/

		if (moveRight)
		{
			low = mid + 1;
		}
		else
		{
			high = mid;
		}

	}

	//此时low就是应该插入的行号

	int insertIndex = low;

	model->insertRow(insertIndex, newRow);

	QModelIndex newIndex = model->index(insertIndex, 0);

	QModelIndex viewIdx = newIndex;

	QAbstractItemModel * viewModel = view->model();

	//检查view用的是不是代理模型

	if (viewModel!= model)
	{
		QSortFilterProxyModel * proxy = qobject_cast<QSortFilterProxyModel*>(viewModel);
		if (proxy)
		{
			viewIdx = proxy->mapFromSource(newIndex);
		}
	}
	MoveScrollBar(view, viewIdx); 
}

 

hnCommon::hnRoadDiseaseInfo hnDiseaseListWidget::getUserSelectDisease(int index)
{
	return hnRoadDiseaseInfo();
}



QModelIndex hnDiseaseListWidget::getUserSelectIndex(QSortFilterProxyModel * proxy, 
	const hnRoadDiseaseInfo& disease,
	int searchCol /*= 0*/, int role /*= QT::UserRole*/)
{
	if (!proxy || !&disease )
	{
		return QModelIndex();
	} 
	auto sourceModel = proxy->sourceModel();
	if (!sourceModel)
	{
		return QModelIndex();
	}




	//QModelIndexList  matches = sourceModel->match(
	//	sourceModel->index(0, searchCol),
	//	role,
	//	QVariant::fromValue(disease),
	//	1,
	//	Qt::MatchExactly | Qt::MatchRecursive //精准匹配
	//);
	int rowCount = sourceModel->rowCount();
	QModelIndexList matches;
	for (int i = 0;  i < rowCount; ++i)
	{
		QModelIndex matche = sourceModel->index(i, 0);
		hnRoadDiseaseInfo  curDis = matche.data(role).value<hnRoadDiseaseInfo>();
		if (curDis == disease)
		{
			matches.push_back(matche);

			 break;
		} 
	}


	if (matches.isEmpty())
	{
		return QModelIndex();
	}

	QModelIndex sourceIndex = matches.first();

	//源索引修改为代理索引
	QModelIndex proxyIndex = proxy->mapFromSource(sourceIndex);
	return proxyIndex;
}

void hnDiseaseListWidget::MoveScrollBar(QTableView * view, QModelIndex viewIdx)
{
	if (viewIdx.isValid())
	{
		QItemSelectionModel* selectionModel = view->selectionModel();
		selectionModel->setCurrentIndex(viewIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

		QTimer::singleShot(10, [view, viewIdx]() {

			QRect rect = view->visualRect(viewIdx);

			if (rect.isEmpty())
			{
				view->resizeRowToContents(viewIdx.row());
				rect = view->visualRect(viewIdx);

			}

			if (!rect.isEmpty())
			{
				view->scrollTo(viewIdx, QAbstractItemView::EnsureVisible);

			}
			else
			{
				QScrollBar *vBar = view->verticalScrollBar();

				int rowHeight = view->rowHeight(viewIdx.row());

				if (rowHeight <= 0)
				{
					rowHeight = 30;
				}

				int targetY = viewIdx.row()*rowHeight;
				int viewportHeight = view->viewport()->height();
				vBar->setValue(targetY - (viewportHeight / 2));
			}

		});
	}
}
