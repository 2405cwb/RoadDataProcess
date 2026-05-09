#include "StdAfx.h"
#include "HdSceneModel.h"

namespace hd
{
	CHdSceneModel::CHdSceneModel(void)
	{
	}


	CHdSceneModel::~CHdSceneModel(void)
	{
		m_TexturePath.clear();
	}

	// 添加模型文件
	bool CHdSceneModel::AddModelFile(const char* strModelFile)
	{
		// 判断合法性
		if (strModelFile == NULL)
		{
			return false;
		}

		m_ModelImportPath = strModelFile;
		return true;
	}

	// 添加文件文件
	bool CHdSceneModel::AddTextureFile(const char* strTextFile)
	{
		// 判断合法性
		if (strTextFile == NULL)
		{
			return false;
		}

		m_TexturePath.push_back(strTextFile);
		return true;
	}

	// 保存
	void CHdSceneModel::Serialize(const char* strWorkspacePath, TiXmlElement* element, bool bSave)
	{
		if (!strWorkspacePath || !element)
		{
			return;
		}

		if (bSave)
		{
			//TiXmlElement *models = new TiXmlElement("Models");
			//element->LinkEndChild(models);

			TiXmlText* xmlText = NULL;
			TiXmlElement* xmlElement = new TiXmlElement("Model");
			element->LinkEndChild(xmlElement);	


			TiXmlElement* ModelpathEle = new TiXmlElement("ModelPath");
			xmlElement->LinkEndChild(ModelpathEle);	

			// 获取测站模型名称
			string strModelFileName = m_ModelImportPath.substr(m_ModelImportPath.find_last_of('\\') + 1);

			// 记录文件名称
			string strTmpFilename = strModelFileName;
			
			// 记录测站模型文件夹名称
			strModelFileName = strModelFileName.substr(0, strModelFileName.find_last_of('.'));

			int pos = m_ModelImportPath.find(".obj");
			if (pos == -1)
			{
				// dem
				strModelFileName += "_DEM";
			}
			else
			{
				// tin
				strModelFileName += "_TIN";
			}

			// 记录模型文件夹的相对路径
			string strFile = "Models\\" + strModelFileName + "\\";

			// 写进工程文件的路径
			strModelFileName = strFile + strTmpFilename;

			xmlText = new TiXmlText(strModelFileName.c_str());
			ModelpathEle->LinkEndChild(xmlText);

			// 将纹理文件路径写入
			for (int i = 0; i < m_TexturePath.size(); i++)
			{
				TiXmlElement* TextElement = new TiXmlElement("TextureFile");
				xmlElement->LinkEndChild(TextElement);	

				// 纹理文件名称
				string strTextFileName = m_TexturePath[i].substr(m_TexturePath[i].find_last_of('\\') + 1);
				strTextFileName = strFile + strTextFileName;

				xmlText = new TiXmlText(strTextFileName.c_str());
				TextElement->LinkEndChild(xmlText);
			}
			
		}
		else
		{
			// element -- model
			string strValue = element->Value();;
			string strText;

			TiXmlElement* ModelElement = element->FirstChildElement(); // ModelPath

			strValue = ModelElement->Value();
			if (strValue == "ModelPath")
			{
				strText = ModelElement->GetText();
				m_ModelImportPath = strWorkspacePath + strText;
			}


			TiXmlElement* TextTureEle = ModelElement->NextSiblingElement();
			while(TextTureEle)
			{
				strValue = TextTureEle->Value();
				if (strValue == "TextureFile")
				{
					strText = TextTureEle->GetText();
					m_TexturePath.push_back(strWorkspacePath + strText);
				}
				TextTureEle = TextTureEle->NextSiblingElement();
			}
		}
	
	}
}


