#ifndef CONFIGSERVICE_H
#define CONFIGSERVICE_H

#include <QObject>
#include <QHash>
#include <QFile>
#include <QMutex>
#include <QDebug>


#include "hnconfigservice_global.h"
#include "cfgInterface.h"
enum IniCodeType
{
	UTF8,
	GB2312
};

class HNCONFIGSERVICE_EXPORT configService : public QObject,public CfgInterface
{
    Q_OBJECT

public:
    explicit configService(QObject *parent = nullptr);
    static configService* getPtr();

    QString getValue(const QString& section, const QString& key);
    QString getValueDft(const QString& section, const QString& key, const QString& dft);
    int     reloadCfg(QString &fileName);
    int     setValue(const QString& section,const QString& key,const  QString& val);
	//在windows下读写GB232格式的配置文件时，需要设置为GB2312编码
	void setIniCode(const IniCodeType &codeType);

	//判断某个节点在不在
	bool isSectionExist(const QString &section);

public:
    void loadCfg(QString fileName);
public:
	 QString ReadString(const QString& Section, const QString& Key, const QString& Default) override;

	 int ReadInteger(const QString& Section, const QString& Key, int Default) override;

	 bool ReadBool(const QString& Section, const QString& Key, bool Default) override;

	 double ReadDouble(const QString& Section, const QString& Key, double Default) override;

	 void WriteInteger(const QString& Section, const QString& Key, int Value) override;

	 void WriteString(const QString& Section, const QString& Key, const QString& Value) override;

	 void WriteBool(const QString& Section, const QString& Key, bool Value) override;

	 void WriteDouble(const QString& Section, const QString& Key, double Value) override;
private:
    int  updateCfgFile(const QString& section,const QString& key,const  QString& val);
    bool checkFile(QFile &file);
    void clearMap(void);

private:
    QMutex mut;
    QHash<QString, QHash<QString, QString> > cfgMap;
    QString m_cfgFileName;
	IniCodeType m_codeType;
};

#define cfgGetValue(sec,key) configService::getPtr()->getValue(sec,key)
#define cfgGetValueDft(sec,key,dft) configService::getPtr()->getValueDft(sec,key,dft)
#define cfgSetValue(sec,key,val) configService::getPtr()->setValue(sec,key,val)
#define cfgReloadCfg() configService::getPtr()->reloadCfg()


#endif // CONFIGSERVICE_H
