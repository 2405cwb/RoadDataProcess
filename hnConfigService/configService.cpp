#include "configService.h"
#include <QRegExp>


configService *configService::getPtr()
{ 
    static configService config;
    return &config;
}

configService::configService(QObject *parent) : QObject(parent)
{
	this->m_codeType = IniCodeType::UTF8;
}

bool configService::checkFile(QFile &file)
{
    if(file.exists() == false)
    {
        qWarning("config file: %s not exists",file.fileName().toLatin1().data());
        return false;
    }

    if(file.open(QIODevice::ReadOnly | QIODevice::Text) == false)
    {
        qWarning("config file: %s open failed",file.fileName().toLatin1().data());
        return false;
    }

    return true;
}

void configService::loadCfg(QString fileName)
{
    m_cfgFileName = fileName;
    QFile file(fileName);
    QString secname;

    if(checkFile(file) == false) return;

    while(!file.atEnd())
    {
        //QString line = QString(file.readLine()).trimmed().simplified();
		//QString line = QString(QString::fromLocal8Bit(file.readLine()).trimmed().simplified());
		QString line;
		if (m_codeType == IniCodeType::UTF8)
		{
			line = QString( file.readLine());
		}
		else if (m_codeType == IniCodeType::GB2312)
		{
			line = QString(QString::fromLocal8Bit(file.readLine()));
		}

		//去除所有空格
		line = line.replace(QRegExp("\\s+"), "");

        if(line.startsWith("#") || line.isEmpty() || line.isNull())
            continue;

        if(line.startsWith("["))// found section
        {
            secname.clear();
            secname = line.remove('[').remove(']');
        }
        else// kv pair
        {
            QStringList kv = line.split("=");
			if (kv.size() != 2)
			{
				continue;
			}
            QString key = kv.at(0);
            QString value = kv.at(1);
       
            value = value.trimmed();
#if 0
            //去掉两边的引号
            if((*value.begin()== QChar('\"')) && (*(value.end() - 1) == QChar('\"')))
            {
                value.remove(0,1);
                value.chop(1);
            }
#endif

            cfgMap[secname].insert(key,value);
        }
    }
    file.close();
}

QString configService::getValue(const QString& section, const QString& key)
{
    QMutexLocker lock(&mut);

    QHash<QString, QHash<QString, QString> >::const_iterator outside;
    QHash<QString, QString>::const_iterator inside;

    outside = cfgMap.find(section);
    if(outside == cfgMap.end()) return "";

    inside = outside.value().find(key);
    if(inside == outside.value().end()) return "";

    return inside.value();
}


QString configService::getValueDft(const QString& section, const QString& key, const QString& dft)
{
    QMutexLocker lock(&mut);

    QHash<QString, QHash<QString, QString> >::const_iterator outside;
    QHash<QString, QString>::const_iterator inside;

    outside = cfgMap.find(section);
    if(outside == cfgMap.end()) return dft;

    inside = outside.value().find(key);
    if(inside == outside.value().end()) return dft;

    return inside.value();
}

int configService::updateCfgFile(const QString& section,const QString& key,const  QString& val)
{
    QString oldFileName = m_cfgFileName;
    QFile oldfile(oldFileName);
    QString newFileName = m_cfgFileName + ".tmp";
    QFile newfile(newFileName);
    bool isFoundSection = false;

    if(!oldfile.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    if(!newfile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return -2;

    while(!oldfile.atEnd())
    {
        //QString line = QString(oldfile.readLine());

		QString line;

		if (this->m_codeType == IniCodeType::UTF8)
		{
			line = QString(oldfile.readLine());
		}
		else if (this->m_codeType == IniCodeType::GB2312)
		{
			line = QString::fromLocal8Bit(oldfile.readLine());
		}

        if(line.startsWith("[" + section + "]"))
            isFoundSection = true;

        if(isFoundSection && line.startsWith(key))
        {
            line.clear();
            //line = key + "=" + "\"" + val + "\"" +"\n";
            line = key + tr("=%1\n").arg(val);
            isFoundSection = false;
			//break;
        }
        //newfile.write(line.toUtf8());
		if (this->m_codeType == IniCodeType::UTF8)
		{
			newfile.write(line.toUtf8());
		}
		else if(this->m_codeType == IniCodeType::GB2312)
		{
			newfile.write(line.toLocal8Bit());
		}
    }

    oldfile.close();
    newfile.flush();
    newfile.close();
    bool temp =  QFile::remove(oldFileName);
    QFile::rename(newFileName,oldFileName);

    return 0;
}

int configService::setValue(const QString& section,const QString& key,const  QString& val)
{
    QMutexLocker lock(&mut);

    //update file
    int ret = updateCfgFile(section,key,val);
    if(ret < 0)
    {
        qWarning("updateCfgFile failed , ret = %d",ret);
        return -1;
    }

    QHash<QString, QHash<QString, QString> >::iterator outside;
    QHash<QString, QString>::iterator inside;

    outside = cfgMap.find(section);
    if(outside == cfgMap.end())
    {
        qWarning("condfig not found section [%s]",qPrintable(section));
        return -2;
    }

    inside = outside.value().find(key);
    if(inside == outside.value().end())
    {
        qWarning("condfig not found key(%s) in section(%s)",
                   qPrintable(key),qPrintable(section));
        return -3;
    }

    inside.value().clear();
    inside.value() = val;
    return 0;
}

void configService::setIniCode(const IniCodeType & codeType)
{
	this->m_codeType = codeType;
}

bool configService::isSectionExist(const QString & section)
{
	auto iter = cfgMap.find(section);
	if (iter == cfgMap.end())
	{
		return false;
	}
	else
	{
		return true;
	}

	return false;
}


int configService::reloadCfg(QString &fileName)
{
    QMutexLocker lock(&mut);
    clearMap();
    loadCfg(fileName);
    qDebug("config service reload config file success");
    return 0;
}

void configService::clearMap(void)
{
    QHash<QString, QHash<QString, QString> >::iterator outside;

    for(outside = cfgMap.begin(); outside != cfgMap.end(); outside++)
        outside.value().clear();

    cfgMap.clear();
}

QString configService::ReadString(const QString& Section, const QString& Key, const QString& Default)
{
		return	getValueDft(Section, Key, Default);
}

int configService::ReadInteger(const QString& Section, const QString& Key, int Default)
{
	QString temp;
	try
	{
	 	 temp = getValueDft(Section, Key, QString::number(Default));
		 return temp.toInt();
	}
	catch (...)
	{
		 temp = QString("config Key:%0 string value can`t to int").arg(Key);
		throw std::exception(temp.toUtf8().data());
	}
}

bool configService::ReadBool(const QString& Section, const QString& Key, bool Default)
{
	bool temp;
	try
	{
		QString str;
		if (Default)
		{
		     str = getValueDft(Section, Key, "True");
		}
		else
		{
			str = getValueDft(Section, Key, "False");
		}
		if (str.compare( "True") == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
		return false;
	}
	catch (...)
	{
		QString temp = QString("config Key:%0  value can`t convert").arg(Key);
		throw std::exception(temp.toUtf8().data());
	}
}

double configService::ReadDouble(const QString& Section, const QString& Key, double Default)
{
	QString temp;
	try
	{
		temp = getValueDft(Section, Key, QString::number(Default));
		return temp.toDouble();
	}
	catch (...)
	{
		temp = QString("config Key:%0  can`t convert").arg(Key);
		throw std::exception(temp.toUtf8().data());
	}
}

void configService::WriteInteger(const QString& Section, const QString& Key, int Value)
{
	  setValue(Section, Key, QString::number(Value));
}

void configService::WriteString(const QString& Section, const QString& Key, const QString& Value)
{
	setValue(Section, Key, Value);
}

void configService::WriteBool(const QString& Section, const QString& Key, bool Value)
{
	QString temp = Value ? "True" : "False";
	setValue(Section, Key, temp);
}

void configService::WriteDouble(const QString& Section, const QString& Key, double Value)
{
	 setValue(Section, Key, QString::number(Value,'g',15));
}

