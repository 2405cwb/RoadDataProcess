#include "hnProjectManager.h"
#include "hnProject.h"
#include <QDir>
#include <QtCore/qiodevice.h>
#include <QApplication>
namespace hnPro
{
	hnProjectManager::hnProjectManager():m_curProject(NULL)
	{
	}


	hnProjectManager::~hnProjectManager()
	{
	}

	// 添加工程
	bool hnProjectManager::addProject(vector<hnProjectDataInfo> vecProject, QProgressDialog& progress)
	{
		if (vecProject.size() <= 0)
		{
			return false;
		}
		
		for (int i = 0; i < vecProject.size(); i++)
		{
			hnProject* newProject = new hnProject();
			if (!newProject->openProject(vecProject[i]))
			{
				delete newProject;
				newProject = NULL;
				progress.hide();
				return false;
			}
			if (newProject->getProjectType()== PROJECT_TYPE::PROJECT_23D_TYPE|| newProject->getProjectType()== PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				progress.setLabelText(QString::fromLocal8Bit("当前加载工程:%1").arg(newProject->get2DProName()));
			}
			else
			{
				progress.setLabelText(QString::fromLocal8Bit("当前加载工程:%1").arg(newProject->get3DProName()));

			}
			progress.show();
			progress.setValue(i + 1);
			QApplication::processEvents();
			m_vecProject.push_back(newProject);
			//setCurProject(newProject->get2DProName());
		}
		//m_curProject = m_vecProject[0];
		if (m_vecProject.size() <= 0)
		{
			return false;
		}  
	return true;
	}

	// 设置当前工程
	bool hnProjectManager::setCurProject(QString strProjectName)
	{
		for (int i = 0; i < m_vecProject.size(); i++)
		{
			if (m_vecProject[i]->getProjectType() == PROJECT_TYPE::PROJECT_2D_TYPE)
			{
				if (m_vecProject[i]->get2DProName().contains(strProjectName))
				{
					m_curProject = m_vecProject[i];
					m_curProject->setCurProject();
					
					return true;
				}
			}
			else if (m_vecProject[i]->getProjectType() == PROJECT_TYPE::PROJECT_23D_TYPE)
			{
				if (m_vecProject[i]->get2DProName().contains(strProjectName))
				{
					m_curProject = m_vecProject[i];
					m_curProject->setCurProject();
					return true;
				}
			}
			else
			{
				if (m_vecProject[i]->get3DProName().contains(strProjectName))
				{
					m_curProject = m_vecProject[i];
					m_curProject->setCurProject();
					return true;
				}
			}

			
		}

		return false;
	}

	// 获取当前工程
	hnProject* hnProjectManager::getCurProject()
	{
		return m_curProject;
	} 

	// 关闭工程
	void hnProjectManager::closeProject()
	{
		for (int i = 0; i < m_vecProject.size(); i++)
		{
			if (m_vecProject[i])
			{
				m_vecProject[i]->closeProject();
				delete m_vecProject[i];
				m_vecProject[i] = NULL;
			}
		}

		m_vecProject.clear();
	}



	

	void hnProjectManager::initAllProjectMileVector()
	{
		for (auto pro : m_vecProject)
		{
			pro->initMileList();
		}
	}

}
