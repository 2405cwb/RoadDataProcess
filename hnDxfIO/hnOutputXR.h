#include <Windows.h>
#include <stdio.h>
#include <vector>
#include "../hnCommon/hn3dDiseaseDef.h"
#include "../hnCommon/hn3dPointDef.h"
#include "../hnCommon/hn2dDiseaseDef.h"
#define STRING_LEN 64

#ifndef STRUCT_DISEASE_C
#define STRUCT_DISEASE_C
typedef struct Disease_C
{
	int beginTrueMile;				//开始真实桩号
	//char roadNum[STRING_LEN];		//车道号	根据用户界面选择的车道顺序来判断
	int roadNum;					//这里改成int，直接传进来int，就不用转了
	char diseaseType[STRING_LEN];	//病害类型 如：板角断裂
	char diseaseDegree[STRING_LEN]; //病害程度 如：轻、中、重、无
	double rectHeight;				//病害高 （对应表中的病害长）
	double rectWidth;				//病害宽 （对应表中的病害宽）
	double distToCenter;			//病害中心位置
	double diseaseArea;				//病害面积
	double calcHeight;				//病害计算高度（对应表中的病害计算长度）
	double calcWidth;				//病害计算宽度（对应表中的病害计算宽度）
	bool bOnRoad;					//不知道是啥东西
	int nRoadType;
}Disease_C;
#endif
#ifndef STRUCT_GRID_DISEASE_C
#define STRUCT_GRID_DISEASE_C
//分段信息
typedef struct GridDisease_C
{
	char strName[STRING_LEN];		//分段的开始里程和结束里程组成的名字， 比如：开始里程2500，结束里程35000，就是K2+500-K35+000
	char strBegMile[STRING_LEN];	//分段的开始里程 和 double 的区别就是他是字符串,double 是double
	char strEndMile[STRING_LEN];	//分段的结束里程 和 double 的区别就是他是字符串,double 是double
	double dBegMileage;				//分段的开始里程	单位米
	double dEndMileage;				//分段的结束里程 单位米
	double dRoadWidth;				//路面宽度 横向 单位米
	int nRoadTotalNum;				//车道总数，用户界面上选择Excel的时候有几个车道就是几个车道 从1开始
}GridDisease_C;
#endif

//等级公路2018、人工模式输出dxf文件。
//这是个全局函数，供外部调用
extern "C" __declspec(dllexport) bool __stdcall OutputXRDxf(const char* filePath, std::vector<Disease_C> diseases, const GridDisease_C& gridDisease, int direction,int roadType);

//导出病害三维图像
extern "C" __declspec(dllexport) bool __stdcall OutputDisease3dDxf(const char* filePath, std::vector<hnCommon::hn3dDiseaseDef<hnCommon::hn3dPointD>*> diss, int diseaseType);

extern "C" __declspec(dllexport) bool __stdcall OutputDisease2dGpsDxf(const char* filePath, std::vector<hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>*> diss, int diseaseType);

extern "C" __declspec(dllexport) bool __stdcall OutputDiseaseLine3dDxf(const char* filePath, std::vector<hnCommon::hn3dDiseaseLineDef<hnCommon::hn3dPointD>*> diss, int diseaseType);
 
