#ifndef _HN_SYSSETTING_H_
#define _HN_SYSSETTING_H_
#include "hnapplication_global.h"
#include "..\hd3DScene\hd3DSceneDefine.h"
#include "..\hnCommon\hnRoadSysParamDef.h"
using namespace hd;
using namespace hnCommon;

namespace hnApp
{
	// 点云渲染参数
	typedef struct PCD_SHOW_INFO
	{
		// 构造函数
		PCD_SHOW_INFO()
		{
			m_eRenderType = RENDER_BY_INTENSITY;

			m_eRenderSize = E_RS_SIZE1;

			m_dPcdShowDist = 30.0;

			m_n3DShowCnt = 5000000;

			m_nPanoShowCnt = 5000000;
		}

		// 渲染方式
		ENUM_RENDERSTYLE m_eRenderType;

		// 渲染尺寸
		ENUM_RENDERSIZE m_eRenderSize;

		// 显示距离
		double m_dPcdShowDist;

		// 3d视图点云显示阈值
		int m_n3DShowCnt;

		// 全景视图显示阈值
		int m_nPanoShowCnt;

	}hnPcdShowInfo;

	class HNAPPLICATION_EXPORT hnSysSetting
	{
	private:
		// 构造析构函数
		hnSysSetting();
		virtual ~hnSysSetting();

		// 定义一个单实例销毁辅助类
		class hdSettingCleaner
		{
		public:
			hdSettingCleaner(){}
			virtual ~hdSettingCleaner()
			{
				if (m_pSysSetting != NULL)
				{
					delete m_pSysSetting;
					m_pSysSetting = NULL;
				}
			}
		};

	public:
		// 获取单实例
		static hnSysSetting* getSetting();

		// 析构单实例
		static void destroySetting();

		// 获取参数
		hnPcdShowInfo getPcdShowInfo();

		// 设置参数
		void setPcdShowInfo(const hnPcdShowInfo& pcdShowInfo);

		// 获取道路设置参数
		hnRoadSysSetInfo getRoadSysSetInfo();

		// 设置道路参数
		void setRoadSysSetInfo(hnRoadSysSetInfo roadSysSetInfo);

	private:
		// 读取参数
		bool readInfo();

		// 写入参数
		bool writeInfo();

	private:
		static hnSysSetting* m_pSysSetting;

	private:
		// 点云显示配准参数
		hnPcdShowInfo m_pcdShowInfo;

		// 道路设置参数
		hnRoadSysSetInfo m_roadSysSetInfo;


		//

		// 参数文件是否打开
		bool m_bOpen;
	};
}

#endif

