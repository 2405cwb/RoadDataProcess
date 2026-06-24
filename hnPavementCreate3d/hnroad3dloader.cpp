#include "hnroad3dloader.h"
#include <string>

hnRoad3dLoader::hnRoad3dLoader(QObject *parent)
	: QObject(parent)
{

}

hnRoad3dLoader::~hnRoad3dLoader()
{

}

using namespace std;
void hnRoad3dLoader::loadPos( QString strFullPosPath,std::vector<POS_STRUCT_INFO>& vecInfo )
{
	// 检查文件是否存在;
	if (_access(strFullPosPath.toLocal8Bit().data(),0) != 0)
	{
		return;
	}

	// 中间文件用于读取数据;
	char strData[1024];
	memset(strData,0,1024);

	// 读取文件;
	int file_line_count = 0;
	bool bFindData = false;
	g_combine_mutex.lock();
	FILE* ptrFile = fopen(strFullPosPath.toLocal8Bit().data(),"rt");
	while (!feof(ptrFile))
	{
		fgets(strData,1024,ptrFile);
		file_line_count++;
	}
	fclose(ptrFile);
	ptrFile = fopen(strFullPosPath.toLocal8Bit().data(),"rt");

	// 读取第一行数据;
	fgets(strData,1024,ptrFile);
	string strLine = strData;
	int nPos = strLine.find_first_of('.');
	if (nPos > 0 && nPos <= 10)
	{
		bFindData = true;
	}

	// 迭代剔除前面的n行数据;
	while (!bFindData)
	{
		// 读取一行数据;
		memset(strData,0,1024);
		fgets(strData,1024,ptrFile);
		strLine = strData;
		nPos = strLine.find_first_of('.');
		if (nPos > 0 && nPos <= 10)
		{
			bFindData = true;
		}
	}

	// 定义存储数据的vector;
	int nPerSize = 50000;
	int nCount = 0;
	vecInfo.resize(nPerSize);

	// 找到后，进行解析;
	POS_STRUCT_INFO infoTmp;
	bool nSize = infoTmp.serialize(strData);
	if (!nSize)
	{
		fclose(ptrFile);
		g_combine_mutex.unlock();
		return;
	}

	// 第一条记录也要存储;
	vecInfo[nCount] = infoTmp;
	nCount++;

	// 读取获取全部数据;
	while (!feof(ptrFile))
	{
		// 读取数据;
		memset(strData,0,1024);
		fgets(strData,1024,ptrFile);

		// 解析数据;
		POS_STRUCT_INFO info;
		nSize = info.serialize(strData);
		if ( nSize )
		{
			if (nCount > 0)
			{
				if (info.dGpsSecond == vecInfo[nCount-1].dGpsSecond )
				{
					continue;
				}
			}

			vecInfo[nCount] = info;
			nCount++;

			// 容器逐渐扩大;
			if (nCount >= vecInfo.size())
			{
				vecInfo.resize(vecInfo.size() + nPerSize);
			}
		}
		if ( nCount % 5000 == 0)
		{
			setProgress(nCount * 1.0 / file_line_count,"读取POS数据...");
		}
	}

	// 仅将数据读取到内存中，不进行投影转换;
	vecInfo.resize(nCount);
	fclose(ptrFile);
	g_combine_mutex.unlock();

	setProgress(1.0,"POS加载完成");

	return;
}

void hnRoad3dLoader::setProgress( float p,const char* str_msg)
{
	QString str_mgs_t = QString::fromLocal8Bit(str_msg);
	emit progress(p,str_mgs_t);
}

void hnRoad3dLoader::loadRoad3dCamData( QString strRoad3dCamPath,std::vector<PAVEMENT_CAM_SYN_INFO>& vecPaveCam,std::vector<QString>& vecCamImgPath )
{
	// 解析数据;
	hnPavementCamReader pavementReader;
	pavementReader.Open(strRoad3dCamPath.toLocal8Bit().data());
	
	// 获取同步信息数据;
	vecPaveCam = pavementReader.getCamSynInfo();

	// 获取所有DAT影像数据;
	pavementReader.getFullDatPaths(vecCamImgPath);
}

void hnRoad3dLoader::splitRoadData( int splitMod,std::vector<POS_STRUCT_INFO>& vecPosInfo,std::vector<PAVEMENT_CAM_SYN_INFO>& vecPaveCam,std::vector<QString>& vecCamImgPath, std::vector<COMBINE_ROAD_PARAM_INFO>& vecCamData )
{
	// 分割大小为1KM的整数倍，1KM对应125张DAT文件;
	int split_mile = (splitMod + 1) * 125;
	vecCamData.clear();

	// 确定分割段数;
	int split_count = (int)floor(vecCamImgPath.size() / split_mile + 0.5);


	// 先分割同步参数文件;
	int cur_cam_index = 0;
	for (unsigned int n = 0;n < split_count;n++)
	{
		COMBINE_ROAD_PARAM_INFO param_info;
		for (unsigned int m = cur_cam_index; m < vecPaveCam.size();m++)
		{
			int temp = m % (split_mile * 100);
			if (temp == 0)
			{
				cur_cam_index = m;
				break;
			}

			// 记录当前段数据;
			param_info.vecPaveCam.push_back(vecPaveCam[m]);
		}

		// 记录影像路径信息;


		vecCamData.push_back(param_info);
	}
}
