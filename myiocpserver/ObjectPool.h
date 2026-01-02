#pragma once

#include "Lock.h"

template<typename T>
class CObjectPool
{
private:
	const size_t m_InitialPoolSize;
	size_t m_CurrentPoolSize;
	std::vector<T*> m_pooledObjects; // stack 방식으로 사용
	CLock m_lock;
	size_t m_inUseCount;

	// lock비용을 줄이기위해 Lock을 잡지않는다.
	std::vector<T*> MakeNewObjects(size_t count)
	{
		std::vector<T*> newObjects;
		newObjects.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			newObjects.push_back(new T());
		}
		return newObjects;
	}

public:
	CObjectPool(size_t poolSize)
		: m_InitialPoolSize(poolSize)
		, m_inUseCount(0)
		, m_CurrentPoolSize(poolSize)
	{
		m_pooledObjects = MakeNewObjects(m_InitialPoolSize);
	}

	CObjectPool(const CObjectPool&) = delete;
	CObjectPool(CObjectPool&&) = delete;


	~CObjectPool()
	{
		for (T* obj : m_pooledObjects)
		{
			delete obj;
		}
	}

	T* AcquireObject()
	{

		if (m_pooledObjects.empty())
		{
			return nullptr; // Pool exhausted
		}


		T* obj = nullptr;
		AUTO_LOCK(m_lock)
		{
			if (!m_pooledObjects.empty())
			{
				obj = m_pooledObjects.back();
				m_pooledObjects.pop_back();
				++m_inUseCount;
			}

			ASSERT(m_pooledObjects.size() + m_inUseCount == m_CurrentPoolSize);
		}

		if (obj == nullptr) 
		{
			// pool이 비어있을 때, 풀을 새로 할당한다
			// 기본적으로 이 케이스는 없는것이 이상적이다.
			// m_InitialPoolSize 사이즈 자체를 늘리는것을 고민할법하다.

			std::vector<T*> addObjects = MakeNewObjects(m_InitialPoolSize);

			AUTO_LOCK(m_lock)
			{
				m_CurrentPoolSize = m_CurrentPoolSize + m_InitialPoolSize;

				if (m_pooledObjects.capacity() <= m_CurrentPoolSize)
				{
					ASSERT(m_pooledObjects.capacity() == m_CurrentPoolSize);
					m_pooledObjects.reserve(m_pooledObjects.capacity() * 2);
				}
				m_pooledObjects.insert(m_pooledObjects.end(), addObjects.begin(), addObjects.end());

				obj = m_pooledObjects.back();
				m_pooledObjects.pop_back();
				++m_inUseCount;

				ASSERT(m_pooledObjects.size() + m_inUseCount == m_CurrentPoolSize);
			}

		}

		

		ASSERT(obj != nullptr);

		return obj;
	}

	void ReleaseObject(T* obj)
	{
		AUTO_LOCK(m_lock)
		{
			m_pooledObjects.push_back(obj);
			--m_inUseCount;

			ASSERT(m_pooledObjects.size() + m_inUseCount == m_CurrentPoolSize);
		}
	}
};


