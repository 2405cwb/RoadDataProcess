#include "cfgBaseService.h"
#include <QRegExp>

cfgBaseService::cfgBaseService()
{

}

void cfgBaseService::loadCfgFile(const QString fileName)
{
    this->m_cfg.loadCfg(fileName);
}

const QString cfgBaseService::getValue(const QString &section, const QString &key)
{
    return this->m_cfg.getValue(section,key);
}

const QString cfgBaseService::getValueDft(const QString &section, const QString &key, const QString &dftValue)
{
    return this->m_cfg.getValueDft(section,key,dftValue);
}

int cfgBaseService::setValue(const QString &section, const QString &key, const QString &newValue)
{
    return this->m_cfg.setValue(section,key,newValue);
}

