#pragma once

#include "Lock.h"

//#define USING_VECTOR


// * TODO: vector가 괜찮을지 자체 Node기반 구조가 나을지 고민해볼것


#if defined(USING_VECTOR)
// With Vector
template<typename T>
class CObjectPool
{
private:
/*
*/
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
			// 기본적으로 이 케이스는 없는것이 이상적이다.
			// m_InitialPoolSize 사이즈 자체를 늘리는것을 고민할법하다.
			// 
			// pool이 비어있을 때, 풀을 새로 할당한다
			// 락 대기를 없애기 위해 new 호출시 락을 잡지않고 지역변수에 할당한다.
			// 이로 인해, 여러 스레드에서 할당이 추가로 일어날 수 있다
			// 이부분은 고려된 사항이다.

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
#else

/* 	
* 직접 노드 및 리스트를 구현해서 사용한다.
* 단순 연결 리스트이나 lock 고려가 되어있다, 스택 구조로 사용된다.
* 실제 노드들은 dequeue 구조로 메모리에 밀집되어있다.
* 반드시 풀에서 받아간 객체를 반환하여야한다.
* 객체 풀을 위한 자료구조라 범용사용이 불가능하다.
*/


template<typename T>
struct CPoolNode
{
	CPoolNode* m_pNext;
	T m_data;

	CPoolNode() : m_pNext(nullptr)
	{}

	static CPoolNode<T>* GetNodePointer(T* pData)
	{
		return reinterpret_cast<CPoolNode<T>*>(
			reinterpret_cast<char*>(pData) - offsetof(CPoolNode, m_data));
	}
};

static_assert(offsetof(CPoolNode<int>, m_data) == sizeof(void*), "m_data must be at offset 8 in CPoolNode<int>.");


template<typename T>
class CConcurrentStackForPool
{
private:
	// Node구조체를 별도로 두지않고 T데이터 앞부분을 next포인터로 사용한다.

	CLock m_lock;
	CPoolNode<T>* m_pHead;
	CPoolNode<T>* m_pTail;
	size_t m_size;
	
public:
	CConcurrentStackForPool() : m_pHead(nullptr), m_pTail(nullptr), m_size(0)
	{}

	~CConcurrentStackForPool()
	{
		CPoolNode<T>* current = m_pHead;
		while (current != nullptr)
		{
			CPoolNode<T>* next = current->m_pNext;
			delete current;
			current = next;
		}
	}


	// 빈 객체 여러개를 한번에 푸시한다. 객체풀 Pool 초기화할때 효율을 위해 사용된다.
	void PushMultiple(size_t count)
	{
		CPoolNode<T>* pNewHead = nullptr;
		CPoolNode<T>* pNewTail = nullptr;

		for (size_t i = 0; i < count; ++i)
		{
			CPoolNode<T>* newNode = new CPoolNode<T>();
			newNode->m_pNext = nullptr;

			if (pNewHead == nullptr)
			{
				pNewHead = newNode;
				pNewTail = newNode;
			}
			else
			{// default case
				pNewTail->m_pNext = newNode;
				pNewTail = newNode;
			}
		}

		AUTO_LOCK(m_lock)
		{
			pNewTail->m_pNext = m_pHead;
			m_pHead = pNewHead;
			if (m_pTail == nullptr)
			{
				// 기존에 비어있던 경우
				m_pTail = pNewTail;
			}
			m_size += count;
		}
	}

	// DATA포인터가 CNode<T>로 캐스팅 가능해야 한다.
	// 반드시 해당 Queue에서 받아간 객체여야 한다.
	void Push(T *pData)
	{
		CPoolNode<T>* newNode = CPoolNode<T>::GetNodePointer(pData);

		AUTO_LOCK(m_lock)
		{
			++m_size;
			// m_pHead이 nullptr인 경우도 자연스레 처리된다.
			newNode->m_pNext = m_pHead;
			m_pHead = newNode;

			if (newNode->m_pNext == nullptr)
			{
				// 추가 당시 m_pHead가 비어있던 경우
				m_pTail = newNode;
			}
		}
	}

	T *PoPorNull()
	{
		AUTO_LOCK(m_lock)
		{
			if (m_pHead == nullptr)
			{
				// empty case
				ASSERT(m_pTail == nullptr);
				ASSERT(m_size == 0);
				// 할당필요 : 추가적인 메모리 할당까지는 list에서 자체적으로 고려하지않는다.
				return nullptr;
			}
			else
			{
				// default case
				--m_size;
				CPoolNode<T>* node = m_pHead;
				m_pHead = m_pHead->m_pNext;

				if (m_pHead == nullptr)
				{// now empty
					m_pTail = nullptr;
				}
				return &(node->m_data);
			}
		}
	}

	// 별도 락을 잡진않는다.
	size_t GetSize()
	{
		return m_size;
	}

	bool IsEmpty()
	{
		return m_size == 0;
	}
};


template<typename T>
class CObjectPool
{
private:
	const size_t m_InitialPoolSize;
	size_t m_CurrentPoolSize;
	CConcurrentStackForPool<T*> m_pooledObjects; // stack 방식으로 사용

	// using atomic
	std::atomic<size_t> m_inUseCount;

public:
	CObjectPool(size_t poolSize)
		: m_InitialPoolSize(poolSize)
		, m_inUseCount(0)
		, m_CurrentPoolSize(poolSize)
	{
		m_pooledObjects.PushMultiple(m_InitialPoolSize);
	}

	CObjectPool(const CObjectPool&) = delete;
	CObjectPool(CObjectPool&&) = delete;


	~CObjectPool()
	{
		ASSERT(m_inUseCount == 0);
	}

	T* AcquireObject()
	{
		T* obj = nullptr;

	
		obj = m_pooledObjects.PoPorNull();
		for (; obj == nullptr; obj = m_pooledObjects.PoPorNull())
		{
			// 기본적으로 이 케이스는 없는것이 이상적이다.
			// m_InitialPoolSize 사이즈 자체를 늘리는것을 고민할법하다.
			// 
			// 루프를 두번 이상 도는건 일반적으로 말이안된다
			// pool이 비어있을 때, 풀을 새로 할당한다
			// 락 대기를 없애기 위해 세팅시엔 락을 잡지않고 지역변수에 세팅한다.
			// 이로 인해, 여러 스레드에서 할당이 추가로 일어날 수 있다
			// 이부분은 고려된 사항이다.

			m_CurrentPoolSize = m_CurrentPoolSize + m_InitialPoolSize;
			m_pooledObjects.PushMultiple(m_InitialPoolSize);
		}


		//obj = m_pooledObjects.PoPorNull();
		//for (; obj == nullptr; obj = m_pooledObjects.PoPorNull())
		//{
		//	// 기본적으로 이 케이스는 없는것이 이상적이다.
		//	// m_InitialPoolSize 사이즈 자체를 늘리는것을 고민할법하다.
		//	// 
		//	// 루프를 두번 이상 도는건 일반적으로 말이안된다
		//	// pool이 비어있을 때, 풀을 새로 할당한다
		//	// 락 대기를 없애기 위해 세팅시엔 락을 잡지않고 지역변수에 세팅한다.
		//	// 이로 인해, 여러 스레드에서 할당이 추가로 일어날 수 있다
		//	// 이부분은 고려된 사항이다.

		//	m_CurrentPoolSize = m_CurrentPoolSize + m_InitialPoolSize;
		//	m_pooledObjects.PushMultiple(m_InitialPoolSize);
		//}

		++m_inUseCount;
		// multi-thread환경에서 size가 틀어질수있다
		//ASSERT(m_pooledObjects.size() + m_inUseCount == m_CurrentPoolSize);

		ASSERT(obj != nullptr);
		return obj;
	}

	void ReleaseObject(T* obj)
	{
		m_pooledObjects.Push(obj);
		--m_inUseCount;

		// multi-thread환경에서 size가 틀어질수있다.
		//ASSERT(m_pooledObjects.size() + m_inUseCount == m_CurrentPoolSize);
	}
};

#endif // defined(USING_VECTOR)
