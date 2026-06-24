#include "hnRibbonElementManager.h"
hnRibbonElementManager* hnRibbonElementManager::s_instance = nullptr;
hnRibbonElementManager::hnRibbonElementManager():m_delegate(nullptr)
{
    m_delegate = new hnRibbonElementCreateDelegate();
}

hnRibbonElementManager::~hnRibbonElementManager()
{
    if(m_delegate)
    {
        delete m_delegate;
    }
    m_delegate = nullptr;
}

hnRibbonElementManager *hnRibbonElementManager::instance()
{
    if(nullptr == s_instance)
    {
        s_instance = new hnRibbonElementManager();
    }
    return s_instance;
}

hnRibbonElementCreateDelegate *hnRibbonElementManager::delegate()
{
    return m_delegate;
}

void hnRibbonElementManager::setupDelegate(hnRibbonElementCreateDelegate *delegate)
{
    if(m_delegate)
    {
        delete m_delegate;
    }
    m_delegate = delegate;
}
