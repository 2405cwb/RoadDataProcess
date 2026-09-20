#include "mergeAidcDiseases.h"
#include "IsMergeableDisease.h"
#include "mergeTwoDiseases.h"
#include "../hnApplication/hnDataManager.h"
#include "../hnProject/hnProject.h"
#include"hnImportAidcDiseases.h"
#include "../hnCommon/hnRoadStruct.h"
mergeAidcDiseases::mergeAidcDiseases(double roadWidth, int pixHeight, bool isVMirror, bool isDiseaseMap, QObject *parent)
	: QObject(parent), m_roadWidth(roadWidth), m_pixHeight(pixHeight), m_isVMirror(isVMirror), m_isDiseaseMap(isDiseaseMap)
{
}

mergeAidcDiseases::~mergeAidcDiseases()
{
}

QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> mergeAidcDiseases::mergeDiseases(QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> diseases)
{

	QProgressDialog progressDialog;
	progressDialog.setFixedSize(QSize(300, 50));
	progressDialog.setWindowTitle(QStringLiteral("病害导入和合并"));
	progressDialog.setCancelButton(nullptr);
	progressDialog.setAutoClose(true);
	progressDialog.setMaximum(diseases.size());
	progressDialog.setModal(true);
	int progressValue = 0;
	progressDialog.setValue(progressValue);
	progressDialog.show();

	QMap<QString, std::vector<hnCommon::hnRoadDiseaseInfo>> result = diseases;

	for (auto iter = result.begin(); iter != result.end(); iter++)
	{
		QString diseaseType = iter.key();
		auto diseaseInfos = iter.value();
		int diseaseDrawType = iter.value()[0].nDrawType;			// 判断是自动化模式还是人工模式

		if (diseaseDrawType == 1) {
			//自动化模式病害合并：纵向裂缝  修补
			if (!diseaseType.contains(QString::fromLocal8Bit("ZXLF")) && !diseaseType.contains(QString::fromLocal8Bit("XB")))
			{
				continue;
			}

		}
		else if (diseaseDrawType == 0)
		{
			//人工模式病害合并：纵向裂缝 横向裂缝 修补
			if (!diseaseType.contains(QString::fromLocal8Bit("ZXLF")) && !diseaseType.contains(QString::fromLocal8Bit("HXLF")) && !diseaseType.contains(QString::fromLocal8Bit("XB")))
			{
				continue;
			}
		}

		// 根据桩号对病害进行从小到大排序
		std::sort(diseaseInfos.begin(), diseaseInfos.end(), [](hnRoadDiseaseInfo &a, hnRoadDiseaseInfo& b) {
			return a.dDmi < b.dDmi;
		});


		// 根据翻转参数修改自动化模式病害,注意：写入数据库的病害坐标是病害在原图的坐标值，界面图像可能会翻转，但是判断是否能够合并病害时要考虑翻转
		// 而自动识别的病害在数据库中的坐标值都是没有翻转的坐标。读取、判断是否能够合并、映射、写入时都要考虑翻转情况
		std::vector<hnCommon::hnRoadDiseaseInfo> mirroredDisease = transDiseaseWithMirror(diseaseInfos, this->m_isVMirror, m_pixHeight);

#if 0
		std::vector<hnCommon::hnRoadDiseaseInfo > mergeSingleDiseaseResult = this->mergeSingleDisease(diseaseInfos);
		result.insert(diseaseType, mergeSingleDiseaseResult);
#endif
		std::vector<hnCommon::hnRoadDiseaseInfo > mergeSingleDiseaseResult;
		if (diseaseDrawType == 0)		// 人工模式病害合并流程
		{
			mergeSingleDiseaseResult = this->BigFrameMergeSingleDisease(mirroredDisease);

			// 翻转病害，为写入数据库做准备
			std::vector<hnCommon::hnRoadDiseaseInfo> mergeNoMirroredDisease = transDiseaseWithMirror(mergeSingleDiseaseResult, this->m_isVMirror, m_pixHeight);
			result.insert(diseaseType, mergeNoMirroredDisease);

		}
		else if (diseaseDrawType == 1)		// 自动化模式病害合并流程
		{
			// diseaseInfos 是自动识别的病害，mirroredDisease 是考虑翻转之后的病害，合并病害需要使用 mirroredDisease 。最后写入数据库需要再翻转，这个和病害存取方式有关
			// 一种图像上只有一个同类型病害时使用这个流程，比 mergeSingleDiseaseMut 简单很多
			//std::vector<hnCommon::hnRoadDiseaseInfo > mergeSingleDiseaseResult = this->mergeSingleDisease(mirroredDisease);
			// 测试一张图像上有多个病害的合并流程
			mergeSingleDiseaseResult = this->mergeSingleDiseaseMut(mirroredDisease);

			// 翻转病害，为写入数据库做准备
			std::vector<hnCommon::hnRoadDiseaseInfo> mergeNoMirroredDisease = transDiseaseWithMirror(mergeSingleDiseaseResult, this->m_isVMirror, m_pixHeight);
			result.insert(diseaseType, mergeNoMirroredDisease);
		}


		progressValue++;
		progressDialog.setValue(progressValue);
		QApplication::processEvents();
	}

	return result;
}

std::vector<hnCommon::hnRoadDiseaseInfo> mergeAidcDiseases::mergeSingleDisease(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos)
{
	if (diseaseInfos.empty())
	{
		return diseaseInfos;
	}

#pragma  region 测试病害组合函数

	std::vector<std::vector<hnCommon::hnRoadDiseaseInfo >> sortedDiseaseSets;
	// 将病害按照图像进行分组，每张图像上可能有多个病害
	sortDiseaseWithSameDmi(diseaseInfos, sortedDiseaseSets);

#pragma endregion


	// 使用对话框设置的阈值
	const double YThreshold = hnApp::hnDataManager::getDataManager()->vMergeLittleFrameThr;				// 单位：像素
	const double leftRightXpixelThreshold = hnApp::hnDataManager::getDataManager()->hMergeLittleFrameThr;			// 单位：像素

	QVector<hnCommon::hnRoadDiseaseInfo> result;
	QVector<hnCommon::hnRoadDiseaseInfo> allDiseaseSets;				// 合并之后病害集合
	QVector<hnCommon::hnRoadDiseaseInfo> addSingleDiseaseSets;			// 记录所有需要添加的病害

	// 获取数据库中病害最大编号，新增病害加到后面
	int maxID = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getDiseaseTable()->getMaxID(diseaseInfos[0].strDiseaseTableName);
	int newID = maxID + 1;

	// 生成两张相邻大图的自动化模式集合
	int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	hnImportAidcDiseases importDisease(frameType, nullptr);
	QVector<QRect> upRectSet = importDisease.createSingleImageLittleFrameRect();
	QVector<QRect> downRectSet = upRectSet;
	for (int i = 0; i < downRectSet.size(); i++)
	{
		downRectSet[i].translate(QPoint(0, m_pixHeight));			// 下方的矩形框坐标加上图像高度，相当于将上下两张图组合成一张大图
	}
	QVector<QRect> BigImageRectSet = downRectSet + upRectSet;			// 两张图矩形框合并

	int baseDiseaseIndex = 0;				// 要合并的最底部的图
	int currentDiseaseIndex = 0;			// 被合入的最新的一张图，一下张图和这张图判断合并
	int nextDiseaseIndex = 1;				// 下一张将要合并的图
	for (baseDiseaseIndex = 0; baseDiseaseIndex < diseaseInfos.size(); /*baseDiseaseIndex++*/)
	{
		currentDiseaseIndex = baseDiseaseIndex;
		nextDiseaseIndex = baseDiseaseIndex + 1;

		addSingleDiseaseSets.clear();
		addSingleDiseaseSets.push_back(diseaseInfos[baseDiseaseIndex]);
		for (currentDiseaseIndex = baseDiseaseIndex; currentDiseaseIndex < diseaseInfos.size(); /*currentDiseaseIndex++*/)
		{
			hnCommon::hnRoadDiseaseInfo current = diseaseInfos[currentDiseaseIndex];
			nextDiseaseIndex = currentDiseaseIndex + 1;
			if (nextDiseaseIndex >= diseaseInfos.size())
			{
				// 将几个可以合并的病害组合在一起,这里处理的是最后一个合并的病害
				hnCommon::hnRoadDiseaseInfo combinedDisease;
				bool succ = combineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
				if (succ)
				{
					allDiseaseSets.push_back(combinedDisease);
					newID++;
				}

				baseDiseaseIndex = nextDiseaseIndex;
				break;
			}
			hnCommon::hnRoadDiseaseInfo next = diseaseInfos[nextDiseaseIndex];
			IsMergeableDisease judgeMergeDiseases;
			int upMinDisIndex = 0, downMinDisIndex = 0;
			const bool isMergeable = judgeMergeDiseases.IsMergeable(current, next, this->m_isVMirror, m_pixHeight, m_roadWidth, upMinDisIndex, downMinDisIndex, YThreshold, leftRightXpixelThreshold);
			if (isMergeable)
			{
				// 将两个病害合并
				// ...
				// addSingleFrameSets 添加自动化模式


				// 计算最短距离的两个矩形的中心点
				QPoint UpPoint((next.vec2dRect[upMinDisIndex].p0.x + next.vec2dRect[upMinDisIndex].p2.x) / 2, (next.vec2dRect[upMinDisIndex].p0.y + next.vec2dRect[upMinDisIndex].p2.y) / 2);
				QPoint DownPoint((current.vec2dRect[downMinDisIndex].p0.x + current.vec2dRect[downMinDisIndex].p2.x) / 2, (current.vec2dRect[downMinDisIndex].p0.y + current.vec2dRect[downMinDisIndex].p2.y) / 2);
				DownPoint += QPoint(0, m_pixHeight);

				QLine minDisLine(UpPoint, DownPoint);			// 两个点的连线

				QVector<QRect> connectRect;
				connectRect.clear();
				getConnectRects(BigImageRectSet, minDisLine, connectRect);

				// 将新增的自动化模式添加到对应的图像的病害中 // 如果添加了新的病害，则使用添加之后的进行距离比较
				hnCommon::hnRoadDiseaseInfo tmpDiseaseDown = addSingleDiseaseSets.last();

				hnCommon::hnRoadDiseaseInfo tmpDiseaseUp = next;
				for (int c = 0; c < connectRect.size(); c++)
				{
					// 大于图像高度，就是下面一张图
					if (connectRect[c].topLeft().y() > m_pixHeight)
					{
						hn2dRectI thisRect = current.vec2dRect[0];
						int dmi = current.vec2dRect[0].p0.m_dmi;			// 事件和桩号与原本病害一致
						double time = current.vec2dRect[0].p0.m_time;
						thisRect.p0 = hn2dPointWithMileI(connectRect[c].topLeft().x(), connectRect[c].topLeft().y() - m_pixHeight, dmi, time);
						thisRect.p1 = hn2dPointWithMileI(connectRect[c].topRight().x(), connectRect[c].topRight().y() - m_pixHeight, dmi, time);
						thisRect.p2 = hn2dPointWithMileI(connectRect[c].bottomRight().x(), connectRect[c].bottomRight().y() - m_pixHeight, dmi, time);		// 注意矩形点的顺序
						thisRect.p3 = hn2dPointWithMileI(connectRect[c].bottomLeft().x(), connectRect[c].bottomLeft().y() - m_pixHeight, dmi, time);

						tmpDiseaseDown.vec2dRect.push_back(thisRect);
					}
					else
					{
						hn2dRectI thisRect = next.vec2dRect[0];
						int dmi = next.vec2dRect[0].p0.m_dmi;			// 事件和桩号与原本病害一致
						double time = next.vec2dRect[0].p0.m_time;
						thisRect.p0 = hn2dPointWithMileI(connectRect[c].topLeft().x(), connectRect[c].topLeft().y(), dmi, time);
						thisRect.p1 = hn2dPointWithMileI(connectRect[c].topRight().x(), connectRect[c].topRight().y(), dmi, time);
						thisRect.p2 = hn2dPointWithMileI(connectRect[c].bottomRight().x(), connectRect[c].bottomRight().y(), dmi, time);
						thisRect.p3 = hn2dPointWithMileI(connectRect[c].bottomLeft().x(), connectRect[c].bottomLeft().y(), dmi, time);

						tmpDiseaseUp.vec2dRect.push_back(thisRect);
					}
				}

				// todo 需要将修改之后的病害加入到向量
				// 考虑到中间的病害会和前后两张图合并，所以会发生变化，需要重新添加
				addSingleDiseaseSets.last() = tmpDiseaseDown;
				addSingleDiseaseSets.push_back(tmpDiseaseUp);


				currentDiseaseIndex++;

			}
			else
			{
				// 处理前面要合并的病害
				// 将几个可以合并的病害组合在一起
				hnCommon::hnRoadDiseaseInfo combinedDisease;
				bool succ = combineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
				if (succ)
				{
					allDiseaseSets.push_back(combinedDisease);
					newID++;
				}

				baseDiseaseIndex = nextDiseaseIndex;
				break;
			}


		}


	}

	return allDiseaseSets.toStdVector();



	// 把给定的病害按照图片顺序排序，QMap(double,QVector<hnCommon::hnRoadDiseaseInfo>) 里程 ，然后是该里程的病害
	QMap<double, QVector<hnCommon::hnRoadDiseaseInfo>> mileDiseaseMap;
	for (auto diseaseInfo : diseaseInfos)
	{
		QVector<hnCommon::hnRoadDiseaseInfo> currentMileDiseases = mileDiseaseMap.value(diseaseInfo.dDmi);

		currentMileDiseases.push_back(diseaseInfo);

		mileDiseaseMap.insert(diseaseInfo.dDmi, currentMileDiseases);
	}


	QMap<double, QVector<hnCommon::hnRoadDiseaseInfo>>::iterator iter;
	for (iter = mileDiseaseMap.begin(); iter != mileDiseaseMap.end(); iter++)
	{
		if ((iter + 1) == mileDiseaseMap.end())
		{
			break;
		}
		QVector<hnCommon::hnRoadDiseaseInfo> currentDiseases = iter.value();

		QVector<hnCommon::hnRoadDiseaseInfo> nextDiseases = (iter + 1).value();

		//for (hnCommon::hnRoadDiseaseInfo &current : currentDiseases)
		for (int i = 0; i < currentDiseases.size(); i++)
		{
			for (hnCommon::hnRoadDiseaseInfo &next : nextDiseases)
			{
				hnCommon::hnRoadDiseaseInfo current = currentDiseases.at(i);
				IsMergeableDisease judgeMergeDiseases;
				int upMinDisIndex = 0, downMinDisIndex = 0;
				const bool isMergeable = judgeMergeDiseases.IsMergeable(current, next, this->m_isVMirror, m_pixHeight, m_roadWidth, upMinDisIndex, downMinDisIndex);

				int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
				hnImportAidcDiseases importDisease(frameType, nullptr);
				// 
				QVector<QRect> upRectSet = importDisease.createSingleImageLittleFrameRect();
				QVector<QRect> downRectSet = upRectSet;
				for (int i = 0; i < downRectSet.size(); i++)
				{
					downRectSet[i].translate(QPoint(0, m_pixHeight));			// 下方的矩形框坐标加上图像高度，相当于将上下两张图组合成一张大图
				}
				QVector<QRect> AllRectSet = downRectSet + upRectSet;			// 两张图矩形框合并

				QPoint QtRectUp(current.vec2dRect[upMinDisIndex].p0.x, current.vec2dRect[upMinDisIndex].p2.y);
				QPoint QtRectDown(next.vec2dRect[downMinDisIndex].p0.x, next.vec2dRect[downMinDisIndex].p2.y);
				QtRectDown += QPoint(0, m_pixHeight);

				QLine minDisLine(QtRectUp, QtRectDown);			// 两个点的连线

				QVector<QRect> connectRect;
				getConnectRects(AllRectSet, minDisLine, connectRect);


				if (isMergeable)
				{
					//1.改变next里面的病害, 改为合并之后的
					mergeTwoDiseases mergeTwo;
					auto merge = mergeTwo.merge(next, current, m_roadWidth, m_pixHeight, m_isVMirror);
					hnCommon::hnProjectSetInfo projectSetInfo = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo();

					next = merge;
					//2.删除current里面的病害
					currentDiseases.remove(i);
					i--;
					//3.更新mileDiseaseMap
					iter.value() = currentDiseases;
					(iter + 1).value() = nextDiseases;
					break;
				}

			}
		}

	}

	for (iter = mileDiseaseMap.begin(); iter != mileDiseaseMap.end(); iter++)
	{
		result += iter.value();
	}

	return result.toStdVector();
}


// 考虑一张图存在多个同类病害的情况
std::vector<hnCommon::hnRoadDiseaseInfo> mergeAidcDiseases::mergeSingleDiseaseMut(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos)
{
	if (diseaseInfos.empty() || diseaseInfos.size()==1)
	{
		return diseaseInfos;
	}

#pragma  region 测试病害组合函数

	std::vector<std::vector<hnCommon::hnRoadDiseaseInfo >> sortedDiseaseSets;
	// 将病害按照图像进行分组，每张图像上可能有多个病害
	sortDiseaseWithSameDmi(diseaseInfos, sortedDiseaseSets);

	typedef std::vector<int> FlagLoc;			// 第一个元素存储是否被使用过，第二第三存储种子病害索引，可用于判断多个病害合并到一个
	std::vector<std::vector<FlagLoc>> mergedInfo;			// 记录病害是否被合并过
	std::vector<FlagLoc> info;
	FlagLoc t;
	for (int i = 0; i < sortedDiseaseSets.size(); i++)
	{
		info.clear();
		for (int j = 0; j < sortedDiseaseSets[i].size(); j++)
		{
			t.clear();
			t.push_back(0); t.push_back(-1); t.push_back(-1);	// 初始时病害没有被添加，后面两个数据记录病害被合并到哪个种子病害中
																//t.push_back(0); t.push_back(i); t.push_back(j);	
			info.push_back(t);
		}
		mergedInfo.push_back(info);
	}

#pragma endregion


	// 使用对话框设置的阈值
	const double YThreshold = hnApp::hnDataManager::getDataManager()->vMergeLittleFrameThr;				// 单位：像素
	const double leftRightXpixelThreshold = hnApp::hnDataManager::getDataManager()->hMergeLittleFrameThr;			// 单位：像素

	QVector<hnCommon::hnRoadDiseaseInfo> result;
	QVector<hnCommon::hnRoadDiseaseInfo> allDiseaseSets;				// 合并之后病害集合
	QVector<hnCommon::hnRoadDiseaseInfo> addSingleDiseaseSets;			// 记录所有需要添加的病害
	QVector<hnCommon::hnRoadDiseaseInfo> nextSingleDiseaseSets;			// 临时变量

	// 获取数据库中病害最大编号，新增病害加到后面
	int maxID = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getDiseaseTable()->getMaxID(diseaseInfos[0].strDiseaseTableName);
	int newID = maxID + 1;

	// 生成两张相邻大图的自动化模式集合
	int frameType = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().nDrawType;
	hnImportAidcDiseases importDisease(frameType, nullptr);
	QVector<QRect> upRectSet = importDisease.createSingleImageLittleFrameRect();
	QVector<QRect> downRectSet = upRectSet;
	for (int i = 0; i < downRectSet.size(); i++)
	{
		downRectSet[i].translate(QPoint(0, m_pixHeight));			// 下方的矩形框坐标加上图像高度，相当于将上下两张图组合成一张大图
	}
	QVector<QRect> BigImageRectSet = downRectSet + upRectSet;			// 两张图矩形框合并

	int baseDiseaseIndex = 0;				// 要合并的最底部的图
	int currentDiseaseIndex = 0;			// 被合入的最新的一张图，一下张图和这张图判断合并
	int nextDiseaseIndex = 1;				// 下一张将要合并的图

	int baseDiseaseCol = 0;					// 种子病害所在的列
	int currentDiseaseCol = 0;				// 当前病害所在的列
	int nextDiseaseCol = 0;					// 下一个用于判断的病害所在的列
	FlagLoc flagLoc;
	for (baseDiseaseIndex = 0; baseDiseaseIndex < sortedDiseaseSets.size(); baseDiseaseIndex++)
	{
		currentDiseaseIndex = baseDiseaseIndex;
		nextDiseaseIndex = baseDiseaseIndex + 1;

		// 遍历同一张图上的所有病害
		for (baseDiseaseCol = 0; baseDiseaseCol < sortedDiseaseSets[baseDiseaseIndex].size(); baseDiseaseCol++)
		{
			if (mergedInfo[baseDiseaseIndex][baseDiseaseCol][0] == 1)			// 当前病害已经被添加过，则不进行合并
			{
				continue;
			}

			addSingleDiseaseSets.clear();
			addSingleDiseaseSets.push_back(sortedDiseaseSets[baseDiseaseIndex][baseDiseaseCol]);
			// 修改标志
			flagLoc.clear();
			flagLoc.push_back(1); flagLoc.push_back(baseDiseaseIndex); flagLoc.push_back(baseDiseaseCol);
			mergedInfo[baseDiseaseIndex][baseDiseaseCol] = flagLoc;

			// 如果种子是最后一组数据，则不能继续生长，直接将 addSingleDiseaseSets 中的病害组合输出。（无法才正确处理最后一组的多个病害和前面的某个病害能够合并的情况）
			nextDiseaseIndex = baseDiseaseIndex + 1;
			if (nextDiseaseIndex >= sortedDiseaseSets.size())
			{
				// 将几个可以合并的病害组合在一起,这里处理的是最后一个合并的病害
				hnCommon::hnRoadDiseaseInfo combinedDisease;
				bool succ = combineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
				if (succ)
				{
					allDiseaseSets.push_back(combinedDisease);
					addSingleDiseaseSets.clear();
					newID++;
				}
				continue;
			}

			for (nextDiseaseIndex = baseDiseaseIndex + 1; nextDiseaseIndex < sortedDiseaseSets.size(); /*nextDiseaseIndex++*/)
			{
				nextSingleDiseaseSets.clear();			// next 图像中满足合并条件的病害
				for (nextDiseaseCol = 0; nextDiseaseCol < sortedDiseaseSets[nextDiseaseIndex].size(); nextDiseaseCol++)
				{
					if (mergedInfo[nextDiseaseIndex][nextDiseaseCol][0] == 1)	// 如果后一张图上的病害已经被合并过，则不再将它合并到其他种子病害
					{
						continue;
					}
					// 新的病害和已经添加的每一个病害进行比较，初始时只有一个种子病害
					for (int existInd = 0; existInd < addSingleDiseaseSets.size(); existInd++)
					{
						// 将新增的自动化模式添加到对应的图像的病害中 // 如果添加了新的病害，则使用添加之后的进行距离比较
						hnCommon::hnRoadDiseaseInfo tmpDiseaseDown = addSingleDiseaseSets[existInd];
						hnCommon::hnRoadDiseaseInfo tmpDiseaseUp = sortedDiseaseSets[nextDiseaseIndex][nextDiseaseCol];

						IsMergeableDisease judgeMergeDiseases;
						int upMinDisIndex = 0, downMinDisIndex = 0;
						const bool isMergeable = judgeMergeDiseases.IsMergeable(tmpDiseaseDown, tmpDiseaseUp, this->m_isVMirror, m_pixHeight, m_roadWidth, upMinDisIndex, downMinDisIndex, YThreshold, leftRightXpixelThreshold);
						if (isMergeable)
						{
							// 将两个病害合并
							// 计算最短距离的两个矩形的中心点,P0 是左上角，P2是右下角，使用矩形中心点的连线效果好一些
							QPoint UpPoint((tmpDiseaseUp.vec2dRect[upMinDisIndex].p0.x + tmpDiseaseUp.vec2dRect[upMinDisIndex].p2.x) / 2, (tmpDiseaseUp.vec2dRect[upMinDisIndex].p0.y + tmpDiseaseUp.vec2dRect[upMinDisIndex].p2.y) / 2);
							QPoint DownPoint((tmpDiseaseDown.vec2dRect[downMinDisIndex].p0.x + tmpDiseaseDown.vec2dRect[downMinDisIndex].p2.x) / 2, (tmpDiseaseDown.vec2dRect[downMinDisIndex].p0.y + tmpDiseaseDown.vec2dRect[downMinDisIndex].p2.y) / 2);
							DownPoint += QPoint(0, m_pixHeight);

							QLine minDisLine(UpPoint, DownPoint);			// 两个点的连线

							QVector<QRect> connectRect;
							connectRect.clear();
							getConnectRects(BigImageRectSet, minDisLine, connectRect);

							for (int c = 0; c < connectRect.size(); c++)
							{
								// 大于图像高度，就是下面一张图
								if (connectRect[c].topLeft().y() >= m_pixHeight)
								{
									hn2dRectI thisRect = tmpDiseaseDown.vec2dRect[0];
									int dmi = tmpDiseaseDown.vec2dRect[0].p0.m_dmi;			// 事件和桩号与原本病害一致
									double time = tmpDiseaseDown.vec2dRect[0].p0.m_time;
									thisRect.p0 = hn2dPointWithMileI(connectRect[c].topLeft().x(), connectRect[c].topLeft().y() - m_pixHeight, dmi, time);
									thisRect.p1 = hn2dPointWithMileI(connectRect[c].topRight().x(), connectRect[c].topRight().y() - m_pixHeight, dmi, time);
									thisRect.p2 = hn2dPointWithMileI(connectRect[c].bottomRight().x(), connectRect[c].bottomRight().y() - m_pixHeight, dmi, time);		// 注意矩形点的顺序
									thisRect.p3 = hn2dPointWithMileI(connectRect[c].bottomLeft().x(), connectRect[c].bottomLeft().y() - m_pixHeight, dmi, time);

									tmpDiseaseDown.vec2dRect.push_back(thisRect);
								}
								else
								{
									hn2dRectI thisRect = tmpDiseaseUp.vec2dRect[0];
									int dmi = tmpDiseaseUp.vec2dRect[0].p0.m_dmi;			// 事件和桩号与原本病害一致
									double time = tmpDiseaseUp.vec2dRect[0].p0.m_time;
									thisRect.p0 = hn2dPointWithMileI(connectRect[c].topLeft().x(), connectRect[c].topLeft().y(), dmi, time);
									thisRect.p1 = hn2dPointWithMileI(connectRect[c].topRight().x(), connectRect[c].topRight().y(), dmi, time);
									thisRect.p2 = hn2dPointWithMileI(connectRect[c].bottomRight().x(), connectRect[c].bottomRight().y(), dmi, time);
									thisRect.p3 = hn2dPointWithMileI(connectRect[c].bottomLeft().x(), connectRect[c].bottomLeft().y(), dmi, time);

									tmpDiseaseUp.vec2dRect.push_back(thisRect);
								}
							}

							// 修改标志
							flagLoc.clear();
							flagLoc.push_back(1); flagLoc.push_back(baseDiseaseIndex); flagLoc.push_back(baseDiseaseCol);
							mergedInfo[nextDiseaseIndex][nextDiseaseCol] = flagLoc;

							addSingleDiseaseSets[existInd] = tmpDiseaseDown;			// 这里是修改病害，不是 push,这里push会导致循环次数发生变化
							// 下面这三句只选择一句运行即可，会得到不同的结果
							//nextSingleDiseaseSets.push_back(tmpDiseaseUp);	// 直接 push 会导致某些病害被添加多次
							collectionLittteFrame(nextSingleDiseaseSets, tmpDiseaseUp);			// 如果 nextSingleDiseaseSets 存在 tmpDiseaseUp 则合并，否则添加
							//break;			// 如果下一张图像上的病害和当前已经合并的病害中的一个可以合并，则不继续判断。如果继续判断，可能导致 nextSingleDiseaseSets 重复push某一个病害
						}
					}
				}
				if (nextSingleDiseaseSets.size() > 0)			// 存在新加入的病害，则继续生长
				{
					// todo 可以在这里再用后一张图中的病害寻找前一张图中可以合并的病害，实现多对多的合并
					//addSingleDiseaseSets.insert(addSingleDiseaseSets.end(), nextSingleDiseaseSets.begin(), nextSingleDiseaseSets.end());
					addSingleDiseaseSets += nextSingleDiseaseSets;
					nextSingleDiseaseSets.clear();
					nextDiseaseIndex++;
				}
				else
				{
					// 将几个可以合并的病害组合在一起,这里处理的是不能继续生长的病害
					hnCommon::hnRoadDiseaseInfo combinedDisease;
					bool succ = combineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
					if (succ)
					{
						allDiseaseSets.push_back(combinedDisease);
						addSingleDiseaseSets.clear();
						newID++;
					}

					break;			// 在下一张图像中没有找到可以合并的病害，则退出next的循环，开始查找当前图像的后一个病害
				}
			}
		}
	}

	if (addSingleDiseaseSets.size() > 0) {
		// 将几个可以合并的病害组合在一起,这里处理的是最后一组没有合并的病害
		hnCommon::hnRoadDiseaseInfo combinedDisease;
		bool succ = combineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
		if (succ)
		{
			allDiseaseSets.push_back(combinedDisease);
			addSingleDiseaseSets.clear();
			newID++;
		}
	}

	return allDiseaseSets.toStdVector();
}


void mergeAidcDiseases::getConnectRects(QVector<QRect> &AllRectSet, QLine minDisLine, QVector<QRect> &connectRects)
{
	// 参考：病害合并中的流程  rectAlgorithm::mergeRects
	//QVector<QRect> outRects;
	connectRects.clear();

	for (QRect rect : qAsConst(AllRectSet))
	{
		if (isintersectBetweenRectAndLine(rect, minDisLine))
		{
			connectRects.push_back(rect);
		}
	}

	return;
}


// 判断 SingleDiseaseSets 中是否包含 tmpDiseaseUp 病害，如果不存在，则直接加入。如果存在，则进行自动化模式合并
void mergeAidcDiseases::collectionLittteFrame(QVector<hnCommon::hnRoadDiseaseInfo> &SingleDiseaseSets, hnCommon::hnRoadDiseaseInfo &tmpDisease)
{
	// 如果集合中没有，则直接添加即可
	if (SingleDiseaseSets.isEmpty())
	{
		SingleDiseaseSets.push_back(tmpDisease);
		return;
	}

	// 对向量中的每一个病害，通过 ID 查看某个病害是否被添加过
	for (int i = 0; i < SingleDiseaseSets.size(); i++)
	{
		if (SingleDiseaseSets[i].nID == tmpDisease.nID)
		{
			// 如果 ID 相等，则说明病害已经加入到向量，但是不能保证矩形自动化模式没变，所以要将自动化模式去重后合并
			vector<hn2dRectI> newVec2dRect;
			for (int m = 0; m < tmpDisease.vec2dRect.size(); m++)
			{
				bool existRect = false;
				for (int n = 0; n < SingleDiseaseSets[i].vec2dRect.size(); n++)
				{
					// 通过比较矩形自动化模式的左上角坐标值是否一致，来确定某个方格是否添加到自动化模式集合中
					if (tmpDisease.vec2dRect[m].p0.x == SingleDiseaseSets[i].vec2dRect[n].p0.x&&tmpDisease.vec2dRect[m].p0.y == SingleDiseaseSets[i].vec2dRect[n].p0.y)
					{
						existRect = true;
						break;
					}
				}
				if (existRect == false)
				{
					newVec2dRect.push_back(tmpDisease.vec2dRect[m]);			// 将不存在的自动化模式添加到集合
				}
			}
			SingleDiseaseSets[i].vec2dRect.insert(SingleDiseaseSets[i].vec2dRect.end(), newVec2dRect.begin(), newVec2dRect.end());
			return;
		}
	}

	// 如果上面没有找到相同 ID 的病害，则直接添加
	SingleDiseaseSets.push_back(tmpDisease);

}


bool mergeAidcDiseases::isintersectBetweenRectAndLine(const QRect & rect, const QLineF & line)
{
	QLineF leftLine = QLineF(rect.topLeft(), rect.bottomLeft());
	QLineF rightLine = QLineF(rect.topRight(), rect.bottomRight());
	QLineF topLine = QLineF(rect.topLeft(), rect.topRight());
	QLineF bottomLine = QLineF(rect.bottomLeft(), rect.bottomRight());

	QVector<QLineF> lines;
	lines.push_back(leftLine);
	lines.push_back(rightLine);
	lines.push_back(topLine);
	lines.push_back(bottomLine);

	for (const QLineF &tmpLine : qAsConst(lines))
	{
		if (line.intersect(tmpLine, nullptr) == QLineF::BoundedIntersection)
		{
			return true;
		}
	}

	return false;
}

// 将几个可以合并的病害组合成一个，重新计算参数
bool mergeAidcDiseases::combineMergeableDisease(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int newID, hnCommon::hnRoadDiseaseInfo &newDisease)
{
	// 自动化模式病害去除重叠区域
	int imgWidth = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelX;
	this->merge2dRectWithoutOverlap(addSingleDiseaseSets, this->m_pixHeight, imgWidth);

	hnImportAidcDiseases import(0, nullptr);
	QVector<hnMile> currentMileVector = hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector();

	if (addSingleDiseaseSets.size() <= 0)
	{
		return false;
	}

	newDisease = addSingleDiseaseSets[0];
	newDisease.nID = newID;

	newDisease.vec2dRect.clear();
	newDisease.nRectCnt = newDisease.vec2dRect.size();
	newDisease.vec3dRect.clear();
	newDisease.n3dCnt = newDisease.vec3dRect.size();

	for (int i = 0; i < addSingleDiseaseSets.size(); i++)
	{
		newDisease.vec2dRect.insert(newDisease.vec2dRect.end(), addSingleDiseaseSets[i].vec2dRect.begin(), addSingleDiseaseSets[i].vec2dRect.end());

		if (this->m_isDiseaseMap&&hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
		{
			// 对每个2d病害映射到3D图像上
			QVector<QRect> rect2D;
			rect2D.clear();
			for (hn2dRectI rectI : addSingleDiseaseSets[i].vec2dRect)			// 此处错误使用了 newDisease.vec2dRect 导致重复添加了很多自动化模式病害
			{
				rect2D.push_back(QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y)));
			}
			std::vector<hn3dRectI> rect3D = import.createLittleFrame3dRectIVector(rect2D, addSingleDiseaseSets[i].dDmi);
			newDisease.vec3dRect.insert(newDisease.vec3dRect.end(), rect3D.begin(), rect3D.end());
		}

	}

	//病害面积，每个自动化模式面积固定，都是0.1*0.1
	newDisease.dArea = 0.1 * 0.1 * newDisease.vec2dRect.size();

	// 终点里程
	double dDmiEnd = newDisease.vec2dRect[0].p0.m_dmi;

	//起点里程
	double dDmiStart = newDisease.vec2dRect[0].p0.m_dmi;
	for (hn2dRectI rectI : newDisease.vec2dRect)
	{
		if (rectI.p0.m_dmi > dDmiEnd)
		{
			dDmiEnd = rectI.p0.m_dmi;
		}
		if (rectI.p2.m_dmi < dDmiStart)
		{
			dDmiStart = rectI.p2.m_dmi;
		}
	}
	newDisease.dDmiStart = dDmiStart;
	newDisease.dDmiEnd = dDmiEnd;

	newDisease.dMileage = (dDmiStart + dDmiEnd) / 2;

	return true;
}



// 将几个可以合并的病害组合成一个，重新计算参数
bool mergeAidcDiseases::BigFramecombineMergeableDisease(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int newID, hnCommon::hnRoadDiseaseInfo &newDisease)
{

	hnImportAidcDiseases import(0, nullptr);
	QVector<hnMile> currentMileVector = hnDataManager::getDataManager()->getCurrentProject()->getCurrentMileVector();

	if (addSingleDiseaseSets.size() <= 0)
	{
		return false;
	}

	//获取二维的图片高度
	int pixHeight = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().picPixelY;

	//获取纵向每像素代表多少米
	double scaleY = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioY;
	double scaleX = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRadioX;

	newDisease = addSingleDiseaseSets[0];
	newDisease.nID = newID;
	for (hnRoadDiseaseInfo info : addSingleDiseaseSets)
	{
		if (info.nLevel > newDisease.nLevel)
		{
			newDisease.nLevel = info.nLevel;			// 合并之后的病害等级取几个病害中最大的一个
		}
	}

	newDisease.vec2dRect.clear();
	newDisease.nRectCnt = newDisease.vec2dRect.size();
	newDisease.vec3dRect.clear();
	newDisease.n3dCnt = newDisease.vec3dRect.size();

	// 四个边界点，起始和终止桩号由边界点决定
	hnCommon::hn2dRectI newRectI = addSingleDiseaseSets[0].vec2dRect[0];
	newDisease.dDmi = addSingleDiseaseSets[0].dDmi;
	// 这里不考虑翻转，因为合并之前已经把病害翻转成了和窗口上显示一样的情况，即 p0 在上，p2 在下，且下方都是小里程，所以都是 pixHeight - y
	newDisease.dDmiStart = addSingleDiseaseSets[0].vec2dRect[0].p2.m_dmi + (pixHeight - addSingleDiseaseSets[0].vec2dRect[0].p2.y)*scaleY;
	newDisease.dDmiEnd = addSingleDiseaseSets[0].vec2dRect[0].p0.m_dmi + (pixHeight - addSingleDiseaseSets[0].vec2dRect[0].p0.y)*scaleY;

	for (int i = 0; i < addSingleDiseaseSets.size(); i++)
	{
		//newDisease.vec2dRect.insert(newDisease.vec2dRect.end(), addSingleDiseaseSets[i].vec2dRect.begin(), addSingleDiseaseSets[i].vec2dRect.end());

		// 查找矩形框 x 坐标的最大和最小值
		for (int j = 0; j < addSingleDiseaseSets[i].vec2dRect.size(); j++)
		{
			// 修改下边界 y 坐标
			if (addSingleDiseaseSets[i].dDmi < newRectI.p0.m_dmi)
			{	// 下边界的桩号最小
				newRectI.p2.m_dmi = addSingleDiseaseSets[i].dDmi;
				newRectI.p2.y = addSingleDiseaseSets[i].vec2dRect[j].p2.y;
				newRectI.p3.m_dmi = addSingleDiseaseSets[i].dDmi;
				newRectI.p3.y = addSingleDiseaseSets[i].vec2dRect[j].p3.y;
				// 计算起始里程，注意考虑翻转情况,使用位于底部的 p2 点
				newDisease.dDmiStart = addSingleDiseaseSets[i].vec2dRect[j].p2.m_dmi + (pixHeight - addSingleDiseaseSets[i].vec2dRect[j].p2.y)*scaleY;
			}
			if (qAbs(addSingleDiseaseSets[i].dDmi - newRectI.p0.m_dmi) < 1e-6&&newRectI.p2.y < addSingleDiseaseSets[i].vec2dRect[j].p2.y)
			{	// 桩号相同时比较 y 坐标，y坐标越大越靠下
				newRectI.p2.y = addSingleDiseaseSets[i].vec2dRect[j].p2.y;
				newRectI.p3.y = addSingleDiseaseSets[i].vec2dRect[j].p3.y;
				// 计算起始里程，注意考虑翻转情况，使用位于底部的 p2 点
				newDisease.dDmiStart = addSingleDiseaseSets[i].vec2dRect[j].p2.m_dmi + (pixHeight - addSingleDiseaseSets[i].vec2dRect[j].p2.y)*scaleY;
			}

			// 修改上边界 y 坐标
			if (addSingleDiseaseSets[i].dDmi > newRectI.p0.m_dmi)
			{
				newRectI.p0.m_dmi = addSingleDiseaseSets[i].dDmi;
				newRectI.p0.y = addSingleDiseaseSets[i].vec2dRect[j].p0.y;
				newRectI.p1.m_dmi = addSingleDiseaseSets[i].dDmi;
				newRectI.p1.y = addSingleDiseaseSets[i].vec2dRect[j].p1.y;
				// 计算结束里程，注意考虑翻转情况，使用位于顶部的 p0 点
				newDisease.dDmiEnd = addSingleDiseaseSets[i].vec2dRect[j].p0.m_dmi + (pixHeight - addSingleDiseaseSets[i].vec2dRect[j].p0.y)*scaleY;
			}
			if (qAbs(addSingleDiseaseSets[i].dDmi - newRectI.p0.m_dmi) < 1e-6&&newRectI.p0.y > addSingleDiseaseSets[i].vec2dRect[j].p0.y)
			{
				newRectI.p0.m_dmi = addSingleDiseaseSets[i].dDmi;
				newRectI.p0.y = addSingleDiseaseSets[i].vec2dRect[j].p0.y;
				newRectI.p1.y = addSingleDiseaseSets[i].vec2dRect[j].p1.y;
				// 计算结束里程，注意考虑翻转情况，使用位于顶部的 p0 点
				newDisease.dDmiEnd = addSingleDiseaseSets[i].vec2dRect[j].p0.m_dmi + (pixHeight - addSingleDiseaseSets[i].vec2dRect[j].p0.y)*scaleY;
			}

			// 左侧 x 坐标取最小值
			if (newRectI.p0.x > addSingleDiseaseSets[i].vec2dRect[j].p0.x)
			{
				newRectI.p0.x = addSingleDiseaseSets[i].vec2dRect[j].p0.x;
				newRectI.p3.x = addSingleDiseaseSets[i].vec2dRect[j].p3.x;
			}

			// 右侧 x 坐标取最大值
			if (newRectI.p1.x < addSingleDiseaseSets[i].vec2dRect[j].p1.x)
			{
				newRectI.p1.x = addSingleDiseaseSets[i].vec2dRect[j].p1.x;
				newRectI.p2.x = addSingleDiseaseSets[i].vec2dRect[j].p2.x;
			}
		}
	}

	// 计算人工模式病害的参数
	//newDisease.dDmiStart = newRectI.p2.m_dmi;			// 起始桩号是下边界的桩号
	//newDisease.dDmiEnd = newRectI.p0.m_dmi;				// 结束桩号是上边界的桩号
	newDisease.dMileage = (newDisease.dDmiEnd + newDisease.dDmiStart) / 2;			// 病害里程是起始里程和结束里程的中间值

	// 将新的矩形框保存到病害中
	newDisease.vec2dRect.push_back(newRectI);

	// todo 人工模式病害2d映射到3d
	if (this->m_isDiseaseMap&&hnDataManager::getDataManager()->getCurrentProject()->get3DProject())
	{
		// 2d病害映射到3D图像上
		hn2dRectI rectI = newDisease.vec2dRect[0];
		QRect rect2D = QRect(QPoint(rectI.p0.x, rectI.p0.y), QPoint(rectI.p2.x, rectI.p2.y));
		std::vector<hn3dRectI> rect3D = import.generateLargeFrameHn3dRectVector(rectI);
		newDisease.vec3dRect.insert(newDisease.vec3dRect.end(), rect3D.begin(), rect3D.end());
	}

	// 每张图像代表的长度
	double roudLend = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadLength;
	// 通过桩号确定病害跨越了几张图像
	double imgGapCnt = qAbs(newDisease.vec2dRect[0].p0.m_dmi - newDisease.vec2dRect[0].p2.m_dmi) / roudLend ;

	// 矩形 x y方向像素数
	newDisease.nPixelWid = qAbs(newDisease.vec2dRect[0].p1.x - newDisease.vec2dRect[0].p0.x);
	newDisease.nPixelLen = pixHeight*imgGapCnt - newDisease.vec2dRect[0].p0.y + newDisease.vec2dRect[0].p3.y;

	// 真实尺寸
	newDisease.dLength = newDisease.nPixelLen*scaleY;
	newDisease.dWidth = newDisease.nPixelWid*scaleX;

	//病害面积
/*	newDisease.dArea =*/ 
	hnApp::hnDataManager::getDataManager()->setDiseaseCalcuteSize(newDisease);
	//newDisease.nPixelLen*newDisease.nPixelWid*scaleX*scaleY;

	return true;
}


// 根据翻转情况对病害自动化模式进行翻转
std::vector<hnCommon::hnRoadDiseaseInfo > mergeAidcDiseases::transDiseaseWithMirror(std::vector<hnCommon::hnRoadDiseaseInfo > &disease, bool isVMirror, int imgHeight)
{

	if (disease.size() <= 0)
	{
		return disease;
	}

	std::vector<hnCommon::hnRoadDiseaseInfo > mirroredDisease;
	if (isVMirror)
	{
		for (hnCommon::hnRoadDiseaseInfo &newDisease : disease)
		{
			for (int i = 0; i < newDisease.vec2dRect.size(); i++)
			{
				// 人工模式上下翻转时，p0 变为 p3,  p1 变为 p2
				if (newDisease.nDrawType == 0)
					//if (true)
				{
					hnCommon::hn2dRectI rectI = newDisease.vec2dRect[i];			// 自动是被病害每个框四个点的桩号都是一样的
					// 人工模式翻转时需要确定矩形 p0 、p1和 p3、p3 是否在同一张图像上
					// 两种情况点的桩号都没有变化
					if (qAbs(newDisease.vec2dRect[i].p0.m_dmi - newDisease.vec2dRect[i].p3.m_dmi) < 0.01)
					{
						// 自动识别的人工模式病害，一个病害一定位于一张图像内，翻转后上下边界点p0变为p3,其他类似
						rectI.p3.y = imgHeight - newDisease.vec2dRect[i].p0.y;
						rectI.p2.y = imgHeight - newDisease.vec2dRect[i].p1.y;
						rectI.p1.y = imgHeight - newDisease.vec2dRect[i].p2.y;
						rectI.p0.y = imgHeight - newDisease.vec2dRect[i].p3.y;
					}
					else
					{
						// 合并之后的病害，如果跨图像，则 p0 经过翻转，依然是p0
						rectI.p0.y = imgHeight - newDisease.vec2dRect[i].p0.y;
						rectI.p1.y = imgHeight - newDisease.vec2dRect[i].p1.y;
						rectI.p2.y = imgHeight - newDisease.vec2dRect[i].p2.y;
						rectI.p3.y = imgHeight - newDisease.vec2dRect[i].p3.y;
					}

					newDisease.vec2dRect[i] = rectI;
				}
				else
				{
					hnCommon::hn2dRectI rectI = newDisease.vec2dRect[i];
					// 自动化模式的y坐标进行变换，里程等信息不变
					rectI.p0.y = imgHeight - rectI.p0.y;
					rectI.p1.y = imgHeight - rectI.p1.y;
					rectI.p2.y = imgHeight - rectI.p2.y;
					rectI.p3.y = imgHeight - rectI.p3.y;
					newDisease.vec2dRect[i] = rectI;
				}
			}
			mirroredDisease.push_back(newDisease);
		}
	}
	else
	{
		return disease;
	}

	return mirroredDisease;
}


// 由于每张图像上可能有多个病害，所以这里根据桩号将同一张图像上的病害放到一个病害数组，后续组合病害需要遍历一张图上的所有病害
void mergeAidcDiseases::sortDiseaseWithSameDmi(std::vector<hnCommon::hnRoadDiseaseInfo > &disease, std::vector<std::vector<hnCommon::hnRoadDiseaseInfo >> &sortedDiseaseSets)
{
	sortedDiseaseSets.clear();
	if (disease.size() <= 0)
	{
		return;
	}

	// 对病害排序
	std::sort(disease.begin(), disease.end(), [](hnRoadDiseaseInfo &a, hnRoadDiseaseInfo &b) {
		return a.dDmi < b.dDmi;
	});

	// 保存同一张图上的多个病害
	std::vector<hnCommon::hnRoadDiseaseInfo > onSameImageDisease;

	onSameImageDisease.clear();

	for (int index = 0; index < disease.size(); /*index++*/)
	{
		// 新的图像上的第一个病害直接添加
		if (onSameImageDisease.empty())
		{
			onSameImageDisease.push_back(disease[index]);
			sortedDiseaseSets.push_back(onSameImageDisease);
			index++;
			continue;
		}
		else
		{
			int rowNow = sortedDiseaseSets.size();
			int colNow = sortedDiseaseSets[rowNow - 1].size();
			// 下一个病害和已经被包含到数组中的病害比较桩号，确定是否在同一张图像
			if (abs(disease[index].dDmi - sortedDiseaseSets[rowNow - 1][colNow - 1].dDmi) < 0.001)
			{
				//onSameImageDisease.push_back(disease[index]);
				sortedDiseaseSets[rowNow - 1].push_back(disease[index]);
			}
			else
			{
				onSameImageDisease.clear();				// 桩号不一样，则说明是后续图像上的病害
				continue;
			}
			index++;
		}

	}


}



// 根据几个将要合并的病害，去除重叠自动化模式
void mergeAidcDiseases::merge2dRectWithoutOverlap(QVector<hnCommon::hnRoadDiseaseInfo>& addSingleDiseaseSets, int imgHeight, int imgWidth)
{
	if (addSingleDiseaseSets.size() <= 0)
	{
		return;
	}

	if (addSingleDiseaseSets[0].vec2dRect.size() <= 0)
	{
		return;
	}

	// 自动化模式尺寸的整数值
	int frameSizeWidthI = qAbs(addSingleDiseaseSets[0].vec2dRect[0].p0.x - addSingleDiseaseSets[0].vec2dRect[0].p1.x);
	int frameSizeHeightI = qAbs(addSingleDiseaseSets[0].vec2dRect[0].p0.y - addSingleDiseaseSets[0].vec2dRect[0].p2.y);
	// 由于自动化模式尺寸不一定是图像尺寸的整数倍，所以这里使用精确值定位每个自动化模式的位置
	double frameHeightF = imgHeight*1.0 / (imgHeight / frameSizeHeightI);
	double frameWidthF = imgWidth*1.0 / (imgWidth / frameSizeWidthI);

	int bottomDmi = addSingleDiseaseSets[0].dDmi;
	int topDmi = addSingleDiseaseSets[0].dDmi;
	for (int i = 0; i < addSingleDiseaseSets.size(); i++)
	{
		if (addSingleDiseaseSets[i].dDmi < bottomDmi)
		{
			bottomDmi = addSingleDiseaseSets[i].dDmi;
		}
		if (addSingleDiseaseSets[i].dDmi > topDmi)
		{
			topDmi = addSingleDiseaseSets[i].dDmi;
		}
	}

	// 每张图像对应道路的长度，也等于每张图桩号差值
	double roadLen = hnDataManager::getDataManager()->getCurrentProject()->getCurProSetInfo().dRoadLength;

	QVector<QRectF> bigImageFrame;
	bigImageFrame.clear();
	for (int i = 0; i < addSingleDiseaseSets.size(); i++)
	{
		for (int j = 0; j < addSingleDiseaseSets[i].vec2dRect.size(); j++)
		{
			double dDmi = addSingleDiseaseSets[i].dDmi;
			hn2dRectI rectI = addSingleDiseaseSets[i].vec2dRect[j];
			//if (rectI.p0.x < 0 || rectI.p0.y < 0 || rectI.p2.x > imgWidth || rectI.p2.y > imgHeight)
			//{
			//	continue;			// 越界点检查
			//}
			QPoint pCener((rectI.p0.x + rectI.p2.x) / 2, (rectI.p0.y + rectI.p2.y) / 2);			// 病害矩形框中心点
			// 大图像最顶部的图像行坐标最小，所以这么计算
			pCener.ry() += (topDmi - addSingleDiseaseSets[i].dDmi) / roadLen * imgHeight;			// 根据桩号修改y坐标

			double tr = pCener.y() / frameHeightF;			// 计算中心点所在的矩形索引
			double tc = pCener.x() / frameWidthF;

			// 根据中心点坐标确定点在大矩形上对应的下矿坐标值
			QRectF freamTmp;
			QPoint pt = QPoint(max(std::round(std::floor(tc)*frameWidthF), 0), max(0, std::round(std::floor(tr)*frameHeightF)));
			freamTmp.setTopLeft(pt);
			// y坐标最大值随着桩号变化而变化
			pt = QPoint(min(std::round(std::ceil(tc)*frameWidthF), imgWidth), min(std::round(std::ceil(tr)*frameHeightF), abs(topDmi - addSingleDiseaseSets[i].dDmi + 1)*imgHeight));
			freamTmp.setBottomRight(pt);

			if (bigImageFrame.isEmpty())
			{
				bigImageFrame.push_back(freamTmp);
			}
			else
			{
				bool exist = false;
				for (int k = 0; k < bigImageFrame.size(); k++)
				{
					if (bigImageFrame[k].topLeft() == freamTmp.topLeft())
					{
						exist = true;
						break;
					}
				}
				if (!exist)
				{
					bigImageFrame.push_back(freamTmp);
				}
			}
		}
	}

	for (int i = 0; i < addSingleDiseaseSets.size(); i++)
	{
		// 将病害原本的矩形框清空，重新计算新的自动化模式位置
		std::vector<hnCommon::hn2dRectI> vec2dRectOrigin = addSingleDiseaseSets[i].vec2dRect;
		addSingleDiseaseSets[i].vec2dRect.clear();
		addSingleDiseaseSets[i].nRectCnt = 0;
		addSingleDiseaseSets[i].vec3dRect.clear();
		addSingleDiseaseSets[i].n3dCnt = 0;

		for (int j = 0; j < vec2dRectOrigin.size(); j++)
		{
			double dDmi = addSingleDiseaseSets[i].dDmi;
			hn2dRectI rectI = vec2dRectOrigin[j];
			QPoint pCener((rectI.p0.x + rectI.p2.x) / 2, (rectI.p0.y + rectI.p2.y) / 2);			// 病害矩形框中心点
			// 大图像最顶部的图像行坐标最小，所以这么计算
			pCener.ry() += (topDmi - addSingleDiseaseSets[i].dDmi) / roadLen * imgHeight;			// 根据桩号修改y坐标

			for (int k = 0; k < bigImageFrame.size(); k++)
			{
				if (bigImageFrame[k].contains(pCener))
				{
					// 使用去重后的自动化模式坐标替换原本的小矩形
					int BigtoSingle = (topDmi - addSingleDiseaseSets[i].dDmi) / roadLen * imgHeight;		// 根据桩号计算大图到小图的平移量
					rectI.p0.x = bigImageFrame[k].topLeft().x(); rectI.p0.y = bigImageFrame[k].topLeft().y() - BigtoSingle;
					rectI.p1.x = bigImageFrame[k].topRight().x(); rectI.p1.y = bigImageFrame[k].topRight().y() - BigtoSingle;
					rectI.p2.x = bigImageFrame[k].bottomRight().x(); rectI.p2.y = bigImageFrame[k].bottomRight().y() - BigtoSingle;
					rectI.p3.x = bigImageFrame[k].bottomLeft().x(); rectI.p3.y = bigImageFrame[k].bottomLeft().y() - BigtoSingle;

					addSingleDiseaseSets[i].vec2dRect.push_back(rectI);
					break;
				}
			}

		}
	}


}




std::vector<hnCommon::hnRoadDiseaseInfo> mergeAidcDiseases::BigFrameMergeSingleDisease(std::vector<hnCommon::hnRoadDiseaseInfo> diseaseInfos)
{
	if (diseaseInfos.empty())
	{
		return diseaseInfos;
	}
#pragma  region 测试病害组合函数
	std::vector<std::vector<hnCommon::hnRoadDiseaseInfo >> sortedDiseaseSets;
	// 将病害按照图像进行分组，每张图像上可能有多个病害
	sortDiseaseWithSameDmi(diseaseInfos, sortedDiseaseSets);

	typedef std::vector<int> FlagLoc;			// 第一个元素存储是否被使用过，第二第三存储种子病害索引，可用于判断多个病害合并到一个
	std::vector<std::vector<FlagLoc>> mergedInfo;			// 记录病害是否被合并过
	std::vector<FlagLoc> info;
	FlagLoc t;
	for (int i = 0; i < sortedDiseaseSets.size(); i++)
	{
		info.clear();
		for (int j = 0; j < sortedDiseaseSets[i].size(); j++)
		{
			t.clear();
			t.push_back(0); t.push_back(-1); t.push_back(-1);	// 初始时病害没有被添加，后面两个数据记录病害被合并到哪个种子病害中
																//t.push_back(0); t.push_back(i); t.push_back(j);	
			info.push_back(t);
		}
		mergedInfo.push_back(info);
	}

#pragma endregion

	// 使用对话框设置的阈值
	const double YThreshold = hnApp::hnDataManager::getDataManager()->vMergeLittleFrameThr;				// 单位：像素
	const double leftRightXpixelThreshold = hnApp::hnDataManager::getDataManager()->hMergeLittleFrameThr;			// 单位：像素

	QVector<hnCommon::hnRoadDiseaseInfo> result;
	QVector<hnCommon::hnRoadDiseaseInfo> allDiseaseSets;				// 合并之后病害集合
	QVector<hnCommon::hnRoadDiseaseInfo> addSingleDiseaseSets;			// 记录所有需要添加的病害
	QVector<hnCommon::hnRoadDiseaseInfo> nextSingleDiseaseSets;			// 临时变量

	// 获取数据库中病害最大编号，新增病害加到后面
	int maxID = hnApp::hnDataManager::getDataManager()->getCurrentProject()->getDB()->getDiseaseTable()->getMaxID(diseaseInfos[0].strDiseaseTableName);
	int newID = maxID + 1;

	int baseDiseaseIndex = 0;				// 要合并的最底部的图
	int nextDiseaseIndex = 1;				// 下一张将要合并的图

	int baseDiseaseCol = 0;					// 种子病害所在的列
	int nextDiseaseCol = 0;					// 下一个用于判断的病害所在的列
	FlagLoc flagLoc;

	for (baseDiseaseIndex = 0; baseDiseaseIndex < sortedDiseaseSets.size(); baseDiseaseIndex++)
	{
		nextDiseaseIndex = baseDiseaseIndex + 1;

		// 遍历同一张图上的所有病害
		for (baseDiseaseCol = 0; baseDiseaseCol < sortedDiseaseSets[baseDiseaseIndex].size(); baseDiseaseCol++)
		{
			if (mergedInfo[baseDiseaseIndex][baseDiseaseCol][0] == 1)			// 当前病害已经被添加过，则不进行合并
			{
				continue;
			}

			addSingleDiseaseSets.clear();
			addSingleDiseaseSets.push_back(sortedDiseaseSets[baseDiseaseIndex][baseDiseaseCol]);
			// 修改标志
			flagLoc.clear();
			flagLoc.push_back(1); flagLoc.push_back(baseDiseaseIndex); flagLoc.push_back(baseDiseaseCol);
			mergedInfo[baseDiseaseIndex][baseDiseaseCol] = flagLoc;

			// 如果种子是最后一组数据，则不能继续生长，直接将 addSingleDiseaseSets 中的病害组合输出。（无法才正确处理最后一组的多个病害和前面的某个病害能够合并的情况）
			nextDiseaseIndex = baseDiseaseIndex + 1;
			if (nextDiseaseIndex >= sortedDiseaseSets.size())
			{
				// 将几个可以合并的病害组合在一起,这里处理的是最后一个合并的病害
				hnCommon::hnRoadDiseaseInfo combinedDisease;
				bool succ = BigFramecombineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
				if (succ)
				{
					allDiseaseSets.push_back(combinedDisease);
					addSingleDiseaseSets.clear();
					newID++;
				}
				continue;
			}

			for (nextDiseaseIndex = baseDiseaseIndex + 1; nextDiseaseIndex < sortedDiseaseSets.size(); /*nextDiseaseIndex++*/)
			{
				nextSingleDiseaseSets.clear();			// next 图像中满足合并条件的病害
				for (nextDiseaseCol = 0; nextDiseaseCol < sortedDiseaseSets[nextDiseaseIndex].size(); nextDiseaseCol++)
				{
					if (mergedInfo[nextDiseaseIndex][nextDiseaseCol][0] == 1)	// 如果后一张图上的病害已经被合并过，则不再将它合并到其他种子病害
					{
						continue;
					}
					// 新的病害和已经添加的每一个病害进行比较，初始时只有一个种子病害
					for (int existInd = 0; existInd < addSingleDiseaseSets.size(); existInd++)
					{
						// 将新增的自动化模式添加到对应的图像的病害中 // 如果添加了新的病害，则使用添加之后的进行距离比较
						hnCommon::hnRoadDiseaseInfo tmpDiseaseDown = addSingleDiseaseSets[existInd];
						hnCommon::hnRoadDiseaseInfo tmpDiseaseUp = sortedDiseaseSets[nextDiseaseIndex][nextDiseaseCol];

						IsMergeableDisease judgeMergeDiseases;
						int upMinDisIndex = 0, downMinDisIndex = 0;
						const bool isMergeable = judgeMergeDiseases.BigFrameIsMergeable(tmpDiseaseDown, tmpDiseaseUp,
							m_pixHeight, m_roadWidth, YThreshold, leftRightXpixelThreshold);
						if (isMergeable)
						{
							// todo 将两个病害合并
							// ......


							// 修改标志
							flagLoc.clear();
							flagLoc.push_back(1); flagLoc.push_back(baseDiseaseIndex); flagLoc.push_back(baseDiseaseCol);
							mergedInfo[nextDiseaseIndex][nextDiseaseCol] = flagLoc;

							addSingleDiseaseSets[existInd] = tmpDiseaseDown;			// 这里是修改病害，不是 push,这里push会导致循环次数发生变化
							// 下面这三句只选择一句运行即可，会得到不同的结果
							nextSingleDiseaseSets.push_back(tmpDiseaseUp);	// 直接 push 会导致某些病害被添加多次
							//collectionLittteFrame(nextSingleDiseaseSets, tmpDiseaseUp);			// 如果 nextSingleDiseaseSets 存在 tmpDiseaseUp 则合并，否则添加
							//break;			// 如果下一张图像上的病害和当前已经合并的病害中的一个可以合并，则不继续判断。如果继续判断，可能导致 nextSingleDiseaseSets 重复push某一个病害

						}
					}
				}
				if (nextSingleDiseaseSets.size() > 0)			// 存在新加入的病害，则继续生长
				{
					// todo 可以在这里再用后一张图中的病害寻找前一张图中可以合并的病害，实现多对多的合并
					//addSingleDiseaseSets.insert(addSingleDiseaseSets.end(), nextSingleDiseaseSets.begin(), nextSingleDiseaseSets.end());
					addSingleDiseaseSets += nextSingleDiseaseSets;
					nextSingleDiseaseSets.clear();
					nextDiseaseIndex++;
				}
				else
				{
					// 将几个可以合并的病害组合在一起,这里处理的是不能继续生长的病害
					hnCommon::hnRoadDiseaseInfo combinedDisease;
					bool succ = BigFramecombineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
					if (succ)
					{
						allDiseaseSets.push_back(combinedDisease);
						addSingleDiseaseSets.clear();
						newID++;
					}

					break;			// 在下一张图像中没有找到可以合并的病害，则退出next的循环，开始查找当前图像的后一个病害
				}
			}
		}
	}

	if (addSingleDiseaseSets.size() > 0) {
		// 将几个可以合并的病害组合在一起,这里处理的是最后一组没有合并的病害
		hnCommon::hnRoadDiseaseInfo combinedDisease;
		bool succ = BigFramecombineMergeableDisease(addSingleDiseaseSets, newID, combinedDisease);
		if (succ)
		{
			allDiseaseSets.push_back(combinedDisease);
			addSingleDiseaseSets.clear();
			newID++;
		}
	}

	return allDiseaseSets.toStdVector();

}














