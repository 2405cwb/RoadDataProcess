/*
 * 说明:此类是除了配置文件模块以外，所有模块的基类。
 * 包含:
 * 配置文件模块的对象
 * 加载配置文件接口
 * 获取配置文件内容接口
 * 修改配置文件接口
 */

#ifndef CFGBASESERVICE_H
#define CFGBASESERVICE_H
#include "configService.h"

class HNCONFIGSERVICE_EXPORT  cfgBaseService
{
public:
    cfgBaseService();
/*对外接口*/
public:
    /*
     *函数名称：loadCfgFile
     *函数简介：加载配置文件
     *参数一说明：配置文件名称，要带有绝对路径
     *返回值：无
    */
    void loadCfgFile(const QString fileName);

    /*
     *函数名称：getValue
     *函数简介：获取配置文件的值
     *参数一说明：section名称
     *参数二说明：key名称
     *返回值：value值,如果配置文件中没有，则返回空指针
    */
    const QString getValue(const QString &section,const QString &key);

    /*
     *函数名称：getValueDft
     *函数简介：获取配置文件的值，如果配置文件中没有，则返回默认值
     *参数一说明：section名称
     *参数二说明：key名称
     *参数三说明：默认的value值
     *返回值：value值
    */
    const QString getValueDft(const QString &section,const QString &key,const QString &dftValue);

    /*
     * 函数名称：setValue
     * 函数简介：设置配置文件的值
     * 参数一说明：section名称
     * 参数二说明：key名称
     * 参数三说明：准备设置的value值
     * 返回值：0设置成功 小于0设置失败.
    */

    int setValue(const QString &section,const QString &key,const QString &newValue);


private:
    /*配置文件模块的对象*/
    configService m_cfg;
};

#endif // CFGBASESERVICE_H
