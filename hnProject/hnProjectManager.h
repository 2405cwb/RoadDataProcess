#ifndef _HN_PROJECT_MAMAGER_H_
#define _HN_PROJECT_MAMAGER_H_
#include "hnproject_global.h"
#include "..\hnCommon\hnRoadStruct.h"
#include "..\hnCommon\hnRoadTypeDef.h"
#include <QProgressDialog>
using namespace hnCommon;

namespace hnPro
{
	// 前置声明
	class hnProject;

	class HNPROJECT_EXPORT hnProjectManager
	{
	public:
		hnProjectManager();
		~hnProjectManager();

		// 添加工程
		bool addProject(vector<hnProjectDataInfo> vecProject, QProgressDialog& progress);

		// 设置当前工程
		bool setCurProject(QString strProjectName);

		// 获取当前工程
		hnProject* getCurProject();

		// 关闭工程
		void closeProject();

		vector<hnProject*> getAllBaseProject() { return m_vecProject; }
		
		//一次性为所有工程 初始化桩号数据
		void initAllProjectMileVector();
	private:
		

	private:
		// 工程集合
		vector<hnProject*> m_vecProject;

		// 当前工程
		hnProject* m_curProject;
	};
}

#endif // !_HN_PROJECT_MAMAGER_H_

