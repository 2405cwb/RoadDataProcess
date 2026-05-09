#ifndef HNRIBBONELEMENTMANAGER_H
#define HNRIBBONELEMENTMANAGER_H
#include "hnRibbonElementCreateDelegate.h"
#include "hnQTRibbonGlobal.h"
///
/// \brief 此类是一个全局单例，用于管理SARibbonElementCreateDelegate
///
class HN_QTRIBBON_EXPORT hnRibbonElementManager
{
protected:
	hnRibbonElementManager();
public:
    virtual ~hnRibbonElementManager();
    static hnRibbonElementManager* instance();
	hnRibbonElementCreateDelegate* delegate();
    void setupDelegate(hnRibbonElementCreateDelegate* delegate);

private:
    static hnRibbonElementManager* s_instance;
	hnRibbonElementCreateDelegate* m_delegate;
};
#ifndef RibbonSubElementMgr
#define RibbonSubElementMgr hnRibbonElementManager::instance()
#endif
#ifndef RibbonSubElementDelegate
#define RibbonSubElementDelegate hnRibbonElementManager::instance()->delegate()
#endif
#endif // HNRIBBONELEMENTMANAGER_H
