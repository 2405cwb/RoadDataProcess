#ifndef HNFILE_H
#define HNFILE_H

#include "hnproject_global.h"
#include <QString>
#include <vector>
#include <functional>
#include <string>

using namespace std;

namespace hnPro
{
	class HNPROJECT_EXPORT hnFile
	{

	public:
		hnFile();
		~hnFile();

		//文件是否存在
		bool isFileExist(QString fullName);

		//文件夹是否存在
		bool isFolderExist(const QString &qstrFolder);

		//创建文件夹
		bool createFileName(const QString &FileLoc, const QString &FileName);

		//创建文件夹  有文件夹则提示清空 没有则创建
		bool createFolder(const QString&fullName, bool bErase = true);

		//删除文件夹内容但文件夹保存
		bool removeContent(const QString &folderDir);

		//删除文件夹和文件夹的内容
		bool removeContentAndFolder(const QString &folderDir, float fProgressTwoLevel,
			std::function<void(float, float)>func);

		//获取文件夹类型的文件
		bool getFileType(string FileLoc, string FileType, vector<string>& file_info);

		//获取文件夹类型的文件
		bool getFileType(QString FileLoc, QString FileType, vector<QString>& file_info);
		 

		//获取文件夹类型的文件
		bool getFilePathType(QString FileLoc, QString FileType, QVector<QString>& file_info);

		// 获取指定文件夹下所有文件夹的文件
		bool getAllFolderFile(QString FileLoc, QString FileType, QVector<QString>& file_info);

		//老文件移动至新文件
		void moveOldFile2NewFile(const QString& OldPath,
			const QString& NewPath, vector<QString>&vecFile);

		//老文件复制至新文件
		void copyOldFile2NewFile(const QString& OldPath,
			const QString& NewPath, vector<QString>&vecFile);

		//获得子目录  可以是多个
		QString getChildFolder(const QString& curFolder, vector<QString>&vecFile);

		//获得父目录
		QString getParentFolderPath(const QString& curFolder, QString& qstrParent);

	private:

		//读取txt  0为按行读取 1为按字符读取  利用iostream方式  fgets是c语言接口 速度最快
		//fgets只有遇到换行符才终止 fscanf遇到换行符和空格都会停止
		//读取方式有fgets fgetline fscanf 其中fgets速度最快 他是c的接口 
		//qt方式最好 不用涉及中英文转换  默认为QT格式
		bool ReadTxt(const QString &qstrInfo, vector<QString>&vecTxt, int nType = 0);

		//字符串解析
		void AnalysisTxt(const QString &qstrTxt, vector<QString>&vecContent);

		// 判断文件类型
		int getFileValueType(const QString& strFile);

	};
}

#endif // HNFILE_H
