#pragma once
#include <memory>
#include <vector>

// Destory Checker Ãß°¡
class CSingletonChecker;

class CSingletonDestoryChecker
{
    std::vector<CSingletonChecker*> m_singletonList;
protected:
public:
    CSingletonDestoryChecker() = default;
    virtual ~CSingletonDestoryChecker();
    void AddSingleton(CSingletonChecker* pSingleton);
    void RemoveSingleton(CSingletonChecker* pSingleton);
};

extern CSingletonDestoryChecker g_singletonDestoryChecker;


class CSingletonChecker
{
protected:
    CSingletonChecker();
    virtual ~CSingletonChecker() = default;
};

template <typename T>
class CSingleton : public CSingletonChecker
{
protected:
    CSingleton() = default;
    virtual ~CSingleton() = default;

public:
    CSingleton(const CSingleton&) = delete;
    CSingleton& operator=(const CSingleton&) = delete;

    static void CreateInstance()
    {
        ASSERT(m_pInstance == nullptr);
        m_pInstance.reset(new T());
    }

    static void DestroyInstance()
    {
        g_singletonDestoryChecker.RemoveSingleton(this);
        ASSERT(m_pInstance != nullptr);
        m_pInstance.reset();
        // m_pInstance = nullptr;

    }

    static T& GetInstance()
    {
        ASSERT(m_pInstance != nullptr);
        return *m_pInstance;
    }

protected:
    static std::unique_ptr<T> m_pInstance;
};

template <typename T>
std::unique_ptr<T> CSingleton<T>::m_pInstance = nullptr;

