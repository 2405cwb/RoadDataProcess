#ifndef HNROAD3DLOADER_H
#define HNROAD3DLOADER_H

#include <QObject>
#include <QMutex>
#include "hnDataCombineStructInfo.h"
#include "hnPavementCamReader.h"

extern QMutex g_combine_mutex;

struct COMBINE_ROAD_PARAM_INFO
{
	std::vector<POS_STRUCT_INFO> vecPosInfo;
	std::vector<PAVEMENT_CAM_SYN_INFO> vecPaveCam;
	std::vector<QString> vecCamImgPath;
};

class hnRoad3dLoader : public QObject
{
	Q_OBJECT

public:
	hnRoad3dLoader(QObject *parent);
	~hnRoad3dLoader();

	// 从POS文件中加载POS数据;
	void loadPos(QString strFullPosPath,std::vector<POS_STRUCT_INFO>& vecPosInfo );

	// 从原始3d路面同步文件中加载信息;
	void loadRoad3dCamData(QString strRoad3dCamPath,std::vector<PAVEMENT_CAM_SYN_INFO>& vecPaveCam,std::vector<QString>& vecCamImgPath);

	// 将数据按给定的段宽分割为多个对象;
	void splitRoadData(int splitMod,std::vector<POS_STRUCT_INFO>& vecPosInfo,std::vector<PAVEMENT_CAM_SYN_INFO>& vecPaveCam,std::vector<QString>& vecCamImgPath,
		std::vector<COMBINE_ROAD_PARAM_INFO>& vecCamData);

private:
	// 设置进度信息;
	void setProgress(float,const char*);
signals:
	// 信号标记，主要用于进度处理;
	void progress(float p,QString msg);
};

#endif // HNROAD3DLOADER_H
