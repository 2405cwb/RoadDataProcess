/*! hdResourceBase.h
********************************************************************************
<PRE>
模块名       : hdFramework
文件名       : hdResourceBase.h
相关文件     : 
文件实现功能 : 插件菜单栏、工具栏，数据单元项基类
作者         : 蔡红云、龚书林
版本         : 1.0
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 : 
日 期        版本     修改人              修改内容
2014/02/16   1.0      蔡红云、龚书林  
</PRE>
*******************************************************************************/
#pragma once
#include "stdafx.h"
#include <string>
#include <vector>
namespace hd
{
	namespace fm
	{

		// 命令单元项
		class HDFRAMEWORK_API CHdItemDef
		{
			friend class CHdToolBarDef;
		public:
			CHdItemDef():m_bGroup(FALSE),m_rcID(-1)
			{
				strcpy(m_strCmdID,"");
				m_rcID = -1;
			}
			~CHdItemDef()
			{
				strcpy(m_strCmdID,"");
				m_rcID = -1;
				m_bGroup = FALSE;
			}

			// 是否当前工具开始分组
			BOOL IsGroup()
			{
				return m_bGroup;
			}

			// 命令唯一ID
			char* GetCmdID()
			{
				return m_strCmdID;
			}

			// 设置界面资源ID,mainframe创建工具栏后,每个命令对应一个ID
			void SetRcID(int rcID)
			{
				m_rcID = rcID;
			}

			// 获取界面资源ID
			int GetRcID()
			{

				return m_rcID;
			}
		private:

			// 是否当前工具开始分组
			BOOL m_bGroup;	

			// 命令唯一ID
			char m_strCmdID[256];

			// 界面资源ID
			int  m_rcID;
		};

		// 工具栏
		class HDFRAMEWORK_API CHdToolBarDef 
		{
		public:
			CHdToolBarDef()
			{
				strcpy(m_strName, "");
				strcpy(m_strCaption, "");
				m_rcID = -1;
				m_menuType = 0;
				m_bAttachType = 0;
			}

			CHdToolBarDef(const char* strName, const char* strCaption, int menuType)
			{
				strcpy(m_strName, strName);
				strcpy(m_strCaption, strCaption);
				m_menuType = menuType;
			}

			virtual ~CHdToolBarDef()
			{
				for (std::vector<CHdItemDef*>::iterator it = m_vecCmd.begin();
					it != m_vecCmd.end();it++)
				{
					if (*it != NULL)
					{
						delete *it;
					}
				}
				m_vecCmd.clear();
			}

			// 根据序号获取工具命令,mainframe主程序调用获取菜单内部工具
			CHdItemDef* GetItemInfo(long index)
			{
				if (index < 0 || index >= (long)m_vecCmd.size())
				{
					return NULL;
				}
				return m_vecCmd[index];
			}

			// 获取命令个数
			int GetItemCount()
			{
				return (int)m_vecCmd.size();
			}

			// 获取名称
			char* GetName()
			{
				return m_strName;
			}

			// 获取标题
			char* GetCaption()
			{
				return m_strCaption;
			}

			// 获取界面资源ID
			int GetRcID()
			{
				return m_rcID; 
			}

			// 设置界面资源ID,mainframe创建工具栏后、分配给工具栏的资源ID
			void SetRcID(int rcID)
			{
				m_rcID = rcID;
			}

			// 设置菜单类型还是工具栏类型
			void SetMenuType(int type)
			{
				m_menuType = type;
			}

			// 增加一个命令
			void AddCmd(const char* cmdId, BOOL group)
			{
				// 创建一个数据项对象
				CHdItemDef* pItemDef = new CHdItemDef();

				// 命令ID赋值
				strcpy(pItemDef->m_strCmdID, cmdId);
				pItemDef->m_bGroup = group;
				m_vecCmd.push_back(pItemDef);

			}

			// 获取工具栏的类型
			int GetToolbarDefType()
			{
				return m_menuType;
			}

			// 获取工具栏的依附类型
			int GetToolbarDefAttachType()
			{
				return m_bAttachType;
			}

			// 设置工具栏的依附类型
			void SetToolbarDefAttachType(int attachType)
			{
				m_bAttachType = attachType;
			}

		private:

			// 界面资源ID
			int  m_rcID;

			// 是菜单还是工具栏,0表示工具栏,1表示菜单
			int  m_menuType;

			// 菜单名称
			char m_strName[256];

			// 菜单提示
			char m_strCaption[256];

			// 当前菜单命令列表
			std::vector<CHdItemDef*> m_vecCmd;

			// 是依附于主框架还是依附于面板，0依附于主框架，1依附于面板
			int m_bAttachType;
			
		};
	}
}
