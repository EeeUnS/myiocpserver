#include "pch.h"
#include "CSingleton.h"

CSingletonDestoryChecker g_singletonDestoryChecker;

CSingletonDestoryChecker::~CSingletonDestoryChecker()
{
    ASSERT(!m_singletonList.empty());
}

void CSingletonDestoryChecker::AddSingleton(CSingletonChecker* pSingleton)
{
    m_singletonList.push_back(pSingleton);
}

void CSingletonDestoryChecker::RemoveSingleton(CSingletonChecker* pSingleton)
{
    auto it = std::find(m_singletonList.begin(), m_singletonList.end(), pSingleton);
    if (it != m_singletonList.end())
    {
        m_singletonList.erase(it);
    }
}

CSingletonChecker::CSingletonChecker() {
    g_singletonDestoryChecker.AddSingleton(this);
}
