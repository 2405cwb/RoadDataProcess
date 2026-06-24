#pragma once

#include "hdCommon.h"
#include "..\hdCore\tinyxml.h"
#include <vector>
#include <string>
using namespace std;

namespace hd
{
	class HDCOMMON_API CHdSceneModel
	{
	public:
		CHdSceneModel(void);
		virtual ~CHdSceneModel(void);

	public:
		string			        m_ModelImportPath;	// 模型文件
		vector<string>			m_TexturePath;		// 模型文件下的模型纹理路径

	public:
		// 添加模型文件
		bool AddModelFile(const char* strModelFile);

		// 添加文件文件
		bool AddTextureFile(const char* strTextFile);

		// 保存
		void Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave);
	};
}
