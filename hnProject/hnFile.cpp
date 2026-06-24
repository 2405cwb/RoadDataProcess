#include "hnFile.h"
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <QTextStream>
#include <fstream>
#include <QMessageBox>
#include "hnFileFun.h"

namespace hnPro
{
	hnFile::hnFile()
	{

	}

	hnFile::~hnFile()
	{

	}

	//文件是否存在
	bool hnFile::isFileExist(QString fullName)
	{
		QFileInfo fileInfo(fullName);
		if (fileInfo.isFile())
		{
			return true;
		}

		return false;
	}

	//文件夹是否存在
	bool hnFile::isFolderExist(const QString &qstrFolder)
	{
		QFileInfo dir(qstrFolder);
		if (dir.isDir())
		{
			return true;
		}

		return false;
	}

	//创建文件夹
	bool hnFile::createFileName(const QString &FileLoc, const QString &FileName)
	{
		QString proImagePath = FileLoc + "\\" + FileName;
		QDir proImageFile(proImagePath);
		// 如果没有文件夹则创建文件夹
		if (!proImageFile.exists())
		{
			if (!proImageFile.mkdir(proImagePath))
			{
				return false;
			}
		}
		return true;
	}

	//创建文件夹  有文件夹则提示清空 没有则创建
	bool hnFile::createFolder(const QString&fullName, bool bErase /*= true*/)
	{
		QDir proImageFile(fullName);
		// 如果没有文件夹则创建文件夹
		if (!proImageFile.exists())
		{
			if (!proImageFile.mkdir(fullName))
			{
				QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("创建文件夹失败"),QStringLiteral("确定"));
				return false;
			}
		}
		//存在文件夹
		else
		{
			//不删除 则返回
			if (!bErase)
			{
				return true;
			}
			QMessageBox box(QMessageBox::Warning, QStringLiteral("警告"),
				QStringLiteral("已经存在该文件夹,是否重新生成"));
			box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			box.setButtonText(QMessageBox::Ok, QStringLiteral("是"));
			box.setButtonText(QMessageBox::Cancel, QStringLiteral("否"));
			int ret = box.exec();
			if (ret == QMessageBox::Ok)
			{
				//删除文件夹内容但文件夹保存
				if (!removeContent(fullName))
				{
					QMessageBox::information(NULL, QStringLiteral("提示"), QStringLiteral("删除文件夹失败"));
					return false;
				}

			}
			else
			{
				return false;
			}
		}

		return true;
	}

	//删除文件夹内容但文件夹保存
	bool hnFile::removeContent(const QString &folderDir)
	{
		QDir dir(folderDir);
		QFileInfoList fileList;
		QFileInfo curFile;
		if (!dir.exists()) { return false; }//文件不存，则返回false
		fileList = dir.entryInfoList(QDir::Dirs | QDir::Files
			| QDir::Readable | QDir::Writable
			| QDir::Hidden | QDir::NoDotAndDotDot
			, QDir::Name);
		while (fileList.size() > 0)
		{
			int infoNum = fileList.size();
			for (int i = infoNum - 1; i >= 0; i--)
			{
				curFile = fileList[i];
				if (curFile.isFile())//如果是文件，删除文件
				{
					QFile fileTemp(curFile.filePath());
					fileTemp.remove();
					fileList.removeAt(i);
				}
				if (curFile.isDir())//如果是文件夹
				{
					QDir dirTemp(curFile.filePath());
					QFileInfoList fileList1 = dirTemp.entryInfoList(QDir::Dirs | QDir::Files
						| QDir::Readable | QDir::Writable
						| QDir::Hidden | QDir::NoDotAndDotDot
						, QDir::Name);
					if (fileList1.size() == 0)//下层没有文件或文件夹
					{
						dirTemp.rmdir(".");
						fileList.removeAt(i);
					}
					else//下层有文件夹或文件
					{
						for (int j = 0; j < fileList1.size(); j++)
						{
							if (!(fileList.contains(fileList1[j])))
								fileList.append(fileList1[j]);
						}
					}
				}
			}
		}

	}

	//删除文件夹和文件夹的内容
	bool hnFile::removeContentAndFolder(const QString &folderDir, float fProgressTwoLevel,
		std::function<void(float, float)>func)
	{
		QDir dir(folderDir);
		QFileInfoList fileList;
		QFileInfo curFile;
		if (!dir.exists()) { return false; }//文件不存，则返回false

		fileList = dir.entryInfoList(QDir::Dirs | QDir::Files
			| QDir::Readable | QDir::Writable
			| QDir::Hidden | QDir::NoDotAndDotDot
			, QDir::Name);
		while (fileList.size() > 0)
		{
			int infoNum = fileList.size();
			for (int i = infoNum - 1; i >= 0; i--)
			{
				curFile = fileList[i];
				if (curFile.isFile())//如果是文件，删除文件
				{
					QFile fileTemp(curFile.filePath());
					fileTemp.remove();
					fileList.removeAt(i);
				}
				if (curFile.isDir())//如果是文件夹
				{
					QDir dirTemp(curFile.filePath());
					QFileInfoList fileList1 = dirTemp.entryInfoList(QDir::Dirs | QDir::Files
						| QDir::Readable | QDir::Writable
						| QDir::Hidden | QDir::NoDotAndDotDot
						, QDir::Name);
					if (fileList1.size() == 0)//下层没有文件或文件夹
					{
						dirTemp.rmdir(".");
						fileList.removeAt(i);
					}
					else//下层有文件夹或文件
					{
						for (int j = 0; j < fileList1.size(); j++)
						{
							if (!(fileList.contains(fileList1[j])))
								fileList.append(fileList1[j]);
						}
					}
				}

				//返回双进度条 第一层的值
				func((infoNum - 1 - i + 1)*1.0 / (infoNum - 1), fProgressTwoLevel);
	
			}
		}

		//删除原有的文件夹
		dir.rmdir(folderDir);
		return true;
	}

	//获取文件夹类型的文件
	bool hnFile::getFileType(string FileLoc, string FileType, vector<string>& file_info)
	{
		QString qFilePath = QString::fromLocal8Bit(FileLoc.c_str());

		QString qExt = QString::fromLocal8Bit(FileType.c_str());

		string strPath = "";

		QDir dir(qFilePath);
		if (!dir.exists())
		{
			return false;
		}

		dir.setFilter(QDir::Files);
		QFileInfoList list = dir.entryInfoList();

		for each (QFileInfo fileInfo in list)
		{
			if (!fileInfo.isFile())
			{
				continue;
			}

			if (0 == fileInfo.suffix().compare(qExt, Qt::CaseInsensitive))
			{
				qFilePath = fileInfo.fileName();
				strPath = qFilePath.toLocal8Bit();
				file_info.push_back(strPath);
			}

		}

		if (file_info.empty())
		{
			return false;
		}
		return true;

	}

	

	bool hnFile::getFileType(QString FileLoc, QString FileType, vector<QString>& file_info)
	{
		QString qFilePath = FileLoc;

		QString qExt = FileType;

		string strPath = "";

		QDir dir(qFilePath);
		if (!dir.exists())
		{
			return false;
		}

		dir.setFilter(QDir::Files);
		QFileInfoList list = dir.entryInfoList();

		for each (QFileInfo fileInfo in list)
		{
			if (!fileInfo.isFile())
			{
				continue;
			}

			QString suffix = fileInfo.suffix();
			if (0 == fileInfo.suffix().compare(qExt, Qt::CaseInsensitive))
			{
				qFilePath = fileInfo.fileName();
				file_info.push_back(qFilePath);
			}

		}

		if (file_info.empty())
		{
			return false;
		}
		return true;

	}

	//获取文件夹类型的文件
	bool hnFile::getFilePathType(QString FileLoc, QString FileType, QVector<QString>& file_info)
	{
		QString qFilePath = FileLoc;

		QString qExt = FileType;

		string strPath = "";

		QDir dir(qFilePath);
		if (!dir.exists())
		{
			return false;
		}

		dir.setFilter(QDir::Files);
		QFileInfoList list = dir.entryInfoList();

		for each (QFileInfo fileInfo in list)
		{
			if (!fileInfo.isFile())
			{
				continue;
			}
		 QString temp = 	fileInfo.suffix();
			if (0 == fileInfo.suffix().compare(qExt, Qt::CaseInsensitive))
			{
				qFilePath = fileInfo.filePath();
				file_info.push_back(qFilePath);
			}

		}

		if (file_info.empty())
		{
			return false;
		}

		return true;

	}

	 

	// 获取指定文件夹下所有文件夹的文件
	bool hnFile::getAllFolderFile(QString FileLoc, QString FileType, QVector<QString>& file_info)
	{
		QVector<QString> vecString;
		
		// 获取当前文件夹下所有文件
		getFilePathType(FileLoc, FileType, vecString);

		if (vecString.size() > 0)
		{
			file_info.append(vecString);
		}

		vecString.clear();

		QDir dir(FileLoc);
		FileLoc = dir.fromNativeSeparators(FileLoc);
		if (!dir.exists())
		{
			return true;
		}

		dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
		dir.setSorting(QDir::Name);
		QFileInfoList mImageName = dir.entryInfoList();

		for each (QFileInfo fileInfo in mImageName)
		{
			if (fileInfo.isFile())
			{
				continue;
			}

			vecString.clear();

			// 获取当前文件夹下所有文件
			getFilePathType(fileInfo.filePath(), FileType, vecString);

			if (vecString.size() > 0)
			{
				file_info.append(vecString);
			}
		}
	}

	//老文件移动至新文件
	void hnFile::moveOldFile2NewFile(const QString& OldPath,
		const QString& NewPath, vector<QString>&vecFile)
	{
		if (vecFile.empty())
		{
			return;
		}

		QString qstrOldName = "";
		QString qstrNewName = "";
		for (int i = 0; i < vecFile.size(); i++)
		{
			qstrOldName = OldPath + "\\" + vecFile[i];
			qstrNewName = NewPath + "\\" + vecFile[i];

			//移动至Image下
			QFile::rename(qstrOldName, qstrNewName);
		}

	}

	//老文件复制至新文件
	void hnFile::copyOldFile2NewFile(const QString& OldPath,
		const QString& NewPath, vector<QString>&vecFile)
	{
		if (vecFile.empty())
		{
			return;
		}

		QString qstrOldName = "";
		QString qstrNewName = "";
		for (int i = 0; i < vecFile.size(); i++)
		{
			qstrOldName = OldPath + "\\" + vecFile[i];
			qstrNewName = NewPath + "\\" + vecFile[i];

			//移动至Image下
			QFile::copy(qstrOldName, qstrNewName);
		}

	}

	//获得子文件夹  可以是多个
	QString hnFile::getChildFolder(const QString& curFolder,vector<QString>&vecFile)
	{
		QDir* dir = new QDir(curFolder);
		QStringList filter;
		QList<QFileInfo>*fileInfo = new QList<QFileInfo>(dir->entryInfoList(filter));

		//文件大小
		if (fileInfo->count()==0)
		{
			return QStringLiteral("无法获得子文件夹");
		}

		for (int i = 0; i < fileInfo->count(); i++)
		{
			if (fileInfo->at(i).fileName() == "." || fileInfo->at(i).fileName() == ".." || fileInfo->at(i).fileName().contains("ProjectInfo.xml"))
			{
				continue;
			}
			vecFile.push_back(fileInfo->at(i).fileName());
		}
			
		if (vecFile.empty())
		{
			return QStringLiteral("无法获得子文件夹");
		}

		return "";
	}

	//获得父目录
	QString hnFile::getParentFolderPath(const QString& curFolder, QString& qstrParent)
	{
		QString temp = curFolder;
		QDir dir(temp);
		if (!dir.cdUp())
		{
			return QStringLiteral("无法获得父目录");
		}

		qstrParent = dir.path();
		return "";
	}	


	bool hnFile::ReadTxt(const QString &qstrInfo, vector<QString>&vecTxt, int nType /*= 0*/)
	{
		if (0 == nType)
		{
			QFile f(qstrInfo);

			if (!(f.open(QIODevice::ReadOnly | QIODevice::Text)))
			{
				return false;
			}

			QTextStream qtReadTxt(&f);

			QString qstr = "";
			while (!qtReadTxt.atEnd())
			{
				qstr = qtReadTxt.readLine();
				vecTxt.push_back(qstr);
			}

			//txt为空格 则返回错误
			if (!vecTxt.empty())
			{
				if (vecTxt[0] == "")
				{
					return false;
				}
			}
		}


		if (1 == nType)
		{
			ifstream infile;
			infile.open(qstr2str(qstrInfo).c_str());

			if (!infile.is_open())
			{
				return false;
			}

			std::string strTemp = "";

			while (getline(infile, strTemp))
			{
				vecTxt.push_back(QString::fromLocal8Bit(strTemp.c_str()));
			}
			infile.close();
		}

		if (2 == nType)
		{
			ifstream infile;
			infile.open(qstr2str(qstrInfo).c_str());

			if (!infile.is_open())
			{
				return false;
			}

			std::string strTemp = "";

			while (!infile.eof())
			{
				infile >> strTemp;
				vecTxt.push_back(QString::fromLocal8Bit(strTemp.c_str()));
			}
		}

		if (3 == nType)
		{
			FILE *pFile = fopen(qstr2str(qstrInfo).c_str(), "r");

			char buf[1024] = { 0 };
			while (fgets(buf, 1024, pFile))
			{
				vecTxt.push_back(buf);
			}

			fclose(pFile);
		}

		return true;
	}

	//字符串解析
	void hnFile::AnalysisTxt(const QString &qstrTxt, vector<QString>&vecContent)
	{
		QStringList qstList = qstrTxt.split(",");
		for (int j = 0; j < qstList.size(); j++)
		{
			vecContent.push_back(qstList[j]);
		}
	}

	// 判断文件类型
	int hnFile::getFileValueType(const QString& strFile)
	{
		// xyz读取
		FILE* pFile = NULL;

		// 打开文件
		pFile = fopen(strFile.toLocal8Bit(), "r");
		if (!pFile)
		{
			return -1;
		}

		// 将文件指针放在初始位置
		fseek(pFile, 0, SEEK_SET);
		char str[1024] = { 0 };

		// 索引
		int nIndex = 0;

		// 读取第一行数据
		while (!feof(pFile))
		{
			fgets(str, 1024, pFile);
			break;
		}

		fclose(pFile);

		// 解析字符
		vector<QString> vecRets;
		AnalysisTxt(QString::fromLocal8Bit(str), vecRets);

		if (vecRets.size() == 2)
		{
			return 0;
		}

		if (vecRets.size() == 3)
		{
			return 1;
		}

		return -1;
	}
}






