#pragma once
#include <QString>
#include <QStringList>
#include <QFile>
#include <QTextStream >
#include <QDir>
#include <QTextCodec>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
namespace  MyCommonMethods 
{ 

	static int csharpRoundToInt(double value)
	{
		double intPart = 0.0;
		double frac = std::modf(value, &intPart);

		double absFrac = std::fabs(frac);
		if (absFrac<0.5)
		{
			return static_cast<int>(intPart);
		}
		else if(absFrac>0.5)
		{
			return static_cast<int>(intPart + std::copysign(1.0, value));
		}
		else
		{
			int intValue = static_cast<int>(intPart);
			if (intValue%2==0)
			{
				return intValue;
			}
			else
			{
				return static_cast<int>(intPart + std::copysign(1.0, value));
			}
		}

	}

	//获取或创建用户电脑数据目录
	static QString  GetUserPath()
	{
		QString localAppData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		QString appFolder = QDir::cleanPath(localAppData + QDir::separator() + QStringLiteral("夕睿光电") + QDir::separator() + QStringLiteral("二三维一体化软件"));

		//创建目录
		QDir dir;
		if (!dir.exists(appFolder))
		{
			dir.mkpath(appFolder);
		}
		return appFolder;
	}

	//包含中文内容的QString转换为char
	static  std::string QstringToChar(const  QString& txt)
	{
		auto utfTxt = txt.toLocal8Bit();
		auto txt0 = utfTxt.toStdString();
		return txt0;
	}

static	QString myColumIndexToLetter(int colIndex)
	{
		if (colIndex<1)
		{
			return QString();
		}
		QString result;
		while (colIndex>0)
		{
			int remainder = (colIndex - 1) % 26;
			char letter = 'A' + remainder;
			result.prepend(letter);
			colIndex = (colIndex - 1) / 26;
		}
		return result;
	}


	static double rountToNDecimalPlaces(double value ,int n)
	{
		double factor = std::pow(10.0, n);
		return std::round(value*factor) / factor;
	}


	//二分查找，找到第一个满足  list[i].value >= value的索引
	//二分查找，找到第一个满足  list[i].value <= value的索引
	static int findFirstGreaterOrEqual(int line, const QVector<double>& list, double value)
	{
		int left = 0; 
		int right = list.size() - 1;
		int result = list.size(); //默认值为datas.size() 表示没有满足条件的 
		while (left <= right)
		{
			int mid = left + (right - left) / 2;
			if (line >0)
			{
				if (list[mid] >= value)
				{
					result = mid;
					right = mid - 1;
				}
				else
				{
					left = mid + 1;
				}
			}
			else
			{
				if (list[mid] <= value)
				{
					result = mid;
					right = mid - 1;
				}
				else
				{
					left = mid + 1;
				}
			}
			
		}
		return result;
	}


	//找到指定目录下匹配的所有文件  路径，条件，文件后缀
	static void findFilesWithPattern(const QString &directoryPath, const QString&pattern ,const QString &extension,QList<QString>& result)
	{
		QDir dir(directoryPath);
		 

		QString regexPattern = QRegularExpression::escape(pattern);
		regexPattern.replace(QRegularExpression::escape("*"), R"((.*))");
		QRegularExpression regex(regexPattern);

		QFileInfoList fileList = dir.entryInfoList(QDir::Files|QDir::Dirs |QDir::NoDotAndDotDot);

		for (const QFileInfo & fileInfo: fileList)
		{
			if (fileInfo.isDir())
			{
				findFilesWithPattern(fileInfo.absoluteFilePath(), pattern, extension,result);
			}
			if (regex.match(fileInfo.fileName()).hasMatch() )
			{
				QString temp = extension.mid(1);
				QString fileName = fileInfo.absoluteFilePath();
				QString suffx = fileInfo.suffix();
				if (suffx == temp)
				{
					QString path = fileInfo.absoluteFilePath().replace("\\", "/");
					result.append(fileInfo.absoluteFilePath());

				}
			}
		}
		 
	}

 

	//输出时间
static	void printDateTime(const QDateTime & dateTime)
	{
		qDebug() << "Year" << dateTime.date().year();
		qDebug() << "month" << dateTime.date().month();
		qDebug() << "day" << dateTime.date().day();
		qDebug() << "hour" << dateTime.time().hour();
		qDebug() << "minute" << dateTime.time().minute();
		qDebug() << "second" << dateTime.time().second();
	}

	//移动文件
	static int moveFile(const QString& sourFilePath, const QString&targetFilePath) 
	{
		QFile sourceFile(sourFilePath);
		QFile targetFile(targetFilePath);
		if (sourceFile.exists())
		{
			if (targetFile.exists())
			{
				targetFile.remove();
			}
			if (sourceFile.rename(targetFilePath))
			{
				return 1;
			}
			else
			{
				return 0;
			}

		}
		else
		{
			return 0;
		}
	}
	//删除文件
	static int deleteFile(const QString& filePath )
	{
		 
		QFile targetFile(filePath);
		if (targetFile.exists())
		{
			if (targetFile.exists())
			{
				targetFile.remove();
			}
			return 1;

		}
		else
		{
			return 0;
		}
	}
	//删除文件夹
	static void deleteDirectory(const QString& path)
	{
		QDir dir(path);
		if (!dir.exists())
		{
			return;
		}

		QFileInfoList fileList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
		for(QFileInfo file : fileList)
		{
			if (file.isFile())
			{
				QFile::remove(file.absoluteFilePath());
			}
			else
			{
				deleteDirectory(file.absoluteFilePath());
			}
		}
		dir.rmdir(path);
		 
	}
	//读取文件
	static QStringList ReadAllLines(const QString& filePath)
	{
		QStringList list;
		QFile file(filePath);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
			while (!in.atEnd())
			{
				QString line = in.readLine();
				list.append(line);
			}
			file.close();
		}
		return list;
	}

	//读取文件
	static QStringList ReadAllLines(const QString& filePath,const char* code)
	{
		QStringList list;
		QFile file(filePath);
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file); 
			in.setCodec(code);
			while (!in.atEnd())
			{
				QString line = in.readLine();
				list.append(line);
			}
			file.close();
		}
		return list;
	}
	//写文件
	static 	void writeAllLines(const QString& fileName, const QStringList& stringList, QTextCodec *codec)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream out(&file);
		out.setCodec(codec);
		for each (const QString &line in stringList)
		{
			out << line << '\n';
		}
		file.close();
	}
	//写文件
	static 	void writeAllLines(const QString& fileName, const QVector<QString>& stringList, QTextCodec *codec)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream out(&file);
		out.setCodec(codec);
		for each (const QString &line in stringList)
		{
			out << line << '\n';
		}
		file.close();
	}
	static 	void writeAllLines(const QString& fileName, const QStringList& stringList)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream out(&file);
		out.setCodec(QTextCodec::codecForName("utf-8"));
		for (const QString &line : stringList)
		{
			out << line << '\n';
		}
		file.close();
	}
	static 	void writeAllLines(const QString& fileName, const QVector<QString>& stringList)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream out(&file);
		out.setCodec(QTextCodec::codecForName("utf-8"));
		for each (const QString &line in stringList)
		{
			out << line << '\n';
		}
		file.close();
	}

	static 	void writeAllLines(const QString& fileName, const QString& context)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			return;
		}
		QTextStream out(&file);
		out.setCodec(QTextCodec::codecForName("utf-8"));
		out << context;
		out.flush();
		file.close();
	}


	//获得路径下所有 符合筛选条件的文件绝对地址
      static	QStringList getMyAllDirFile(const QString& path, const QStringList & filters)
	{
		QDir dir(path);
		dir.setFilter(QDir::Files);
		dir.setSorting(QDir::Time);
		dir.setNameFilters(filters);
		QStringList files = dir.entryList();
		for ( int i = 0 ; i<files.size();++i)
		{
			files[i] = dir.filePath(files[i]);//将文件名转换为完整路径名
		}
		return files;
	}

	static  void GetAllIRIFiles(QString prj, int side, QString subfolder, QString ftype,QVector<QString>& filepath)
	  {
		  filepath.clear();
		 QString prjpath =QString("%1\\%2%3").arg(prj).arg(subfolder).arg(side);
		  QDir dir (prjpath);
		 QStringList filesPaths = dir.entryList(QDir::Files|QDir::NoDotAndDotDot);
		 QStringList filesPath;
		 for  (QString var : filesPaths)
		 {
			 filesPath.push_back(dir.absoluteFilePath(var));
		 }
		 std::sort(filesPath.begin(), filesPath.end(),
			 [](const QString&file1Str,const QString &file2Str) 
		 
		 {
			 QFileInfo file1(file1Str);
			 QFileInfo file2(file2Str);
			 return file1.created() < file2.created();
		 }
		 );
		 for (QString str:filesPath)
		 {
			 QFileInfo fileOne(str);
			 QString suf= fileOne.suffix();
			 if (suf==ftype)
			 {filepath.push_back(str);
			 } 
			
		 }
		   
	  }

   //递归创建文件夹
 static QString createMultipleFolders(const QString& path)
 {
	 QDir dir(path);
	 if (dir.exists(path))
	 {
		 return path;
	 }
	 QString parentDir = createMultipleFolders(path.mid(0, path.lastIndexOf('/')));
	 QString dirName = path.mid(path.lastIndexOf('/') + 1);
	 QDir parentPath(parentDir);
	 if (!dirName.isEmpty())
	 {
		 parentPath.mkpath(dirName);
	 }
	 return parentDir + "/" + dirName;
 }
 //保存[]数组到文件
 template<typename T>
 static void saveTArrayToFile(const char* filename, T* array, int length)
 {
	 std::ofstream output(filename);
	 if (output.is_open())
	 {
		 for (int i = 0 ; i<length ;++i)
		 {
			 output << array[i] << "\n";

		 }
		 output.close();

	 }
	 else
	 {
		 std::cerr << "Failed to open file:" << filename << std::endl;
	 }
 }

 //将 K0+000转换为double ,字符串不符合要求返回0.0
 static double convertStakeToDouble(const QString& stake)
 {
	 double result = 0.0;
	 if (stake.isEmpty() || !stake.contains("+"))
	 {
		 return 0.0;
	 }
	//去掉K前缀并分割
	 QString cleaned = stake.mid(1);
	 QStringList parts = cleaned.split('+');
	 if (parts.size()!=2)
	 {
		 return 0.0;
	 }
	 //提取公里数和米数
	 QString kiloStr = parts[0].trimmed();
	 QString meterStr = parts[1].trimmed();

	 bool ok;
	 double kilometers = kiloStr.toDouble(&ok);
	 if (!ok)
	 {
		 return 0.0;
	 }
	 double meters = meterStr.toDouble(&ok);
	 if (!ok)
	 {
		 return 0.0;
	 }
	
	 //计算公里数*1000+米数
	 result = kilometers * 1000 + meters;
	
 
	 return result;
 }

 static QString convertMileToString(double value)
 {
	 QString result;
	 int quotient = static_cast<int>(value / 1000);
	 int rem = static_cast<int>(value) % 1000;
	 if (quotient > 0)
	 {
		 result = "K" + QString::number(quotient).rightJustified(3, '0') + "+";
	 }
	 if (quotient==0)
	 {
		 result = "K" + QString::number(quotient).rightJustified(3, '0') + "+";
	 }
	 result += QString::number(rem).rightJustified(3,'0');
	 return result;
 }
};