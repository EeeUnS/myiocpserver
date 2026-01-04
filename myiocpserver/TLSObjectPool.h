#pragma once

#include "ObjectPool.h"

template<typename T>
struct Segment
{
public:
	static constexpr size_t default_segment_size = 32;
	static size_t SEGMENT_SIZE;
	static void SetSegmentSIZE(size_t size)
	{
		SEGMENT_SIZE = size;
	}

	std::vector<T*> m_Objects;
	Segment() : m_Objects(SEGMENT_SIZE)
	{}

	~Segment()
	{
		for (T* item : m_Objects)
		{
			delete item;
		}
		m_Objects.clear();
	}
};


template<typename T>
struct FullSegment : public Segment<T>
{
	FullSegment()
	{
		for (T*& item : Segment<T>::m_Objects)
		{
			item = new T();
		}
	}
}; 
static_assert(sizeof(FullSegment<int>) == sizeof(Segment<int>), "FullSegment and Segment must have the same size.");


template<typename T>
size_t Segment<T>::SEGMENT_SIZE = Segment<T>::default_segment_size;

// Object가 넘칠때, 부족할때 모두 고려되어야하고 이때 Segment객체가 관리되어야한다
template<typename T>
class CThreadLocalData
{
	Segment<T>* m_pSegment;
	Segment<T>* m_pFilledSegment;
	Segment<T>* m_pEmptySegment;

public:
	CThreadLocalData() : m_pSegment(nullptr), m_pFilledSegment(nullptr), m_pEmptySegment(nullptr)
	{
		//upcasting 
		m_pSegment = reinterpret_cast<Segment<T>*>(GLOBAL_FILLED_SEGMENT_POOL.AcquireObject());
	}

	~CThreadLocalData()
	{
		if (m_pSegment != nullptr)
		{
			GLOBAL_EMPTY_SEGMENT_POOL.ReleaseObject(m_pSegment);
			m_pSegment = nullptr;
		}

		if (m_pFilledSegment != nullptr)
		{
			ASSERT(m_pFilledSegment->m_Objects.size() == Segment<T>::SEGMENT_SIZE);
			//downcasting
			GLOBAL_FILLED_SEGMENT_POOL.ReleaseObject(reinterpret_cast<FullSegment<T>*>(m_pFilledSegment));
			m_pFilledSegment = nullptr;
		}

		if (m_pEmptySegment != nullptr)
		{
			ASSERT(m_pFilledSegment->m_Objects.size() == 0);
			GLOBAL_EMPTY_SEGMENT_POOL.ReleaseObject(m_pEmptySegment);
			m_pEmptySegment = nullptr;
		}
	}

	T* AcquireObject()
	{
		ASSERT(m_pSegment != nullptr);
		if (m_pSegment->m_Objects.empty())
		{
			if (m_pEmptySegment != nullptr)
			{
				// global 풀로 반환한다.
				GLOBAL_EMPTY_SEGMENT_POOL.ReleaseObject(m_pEmptySegment);
				m_pEmptySegment = nullptr;
			}

			ASSERT(m_pEmptySegment == nullptr);
			std::swap(m_pEmptySegment, m_pSegment);
			ASSERT(m_pSegment == nullptr);

			// 현재 Segment가 비어있다 -> 채워진 세그먼트에서 가져온다.

			if (m_pFilledSegment != nullptr)
			{
				std::swap(m_pSegment, m_pFilledSegment);
			}
			else
			{
				// 빈 세그먼트도 없다 -> 새로 할당한다.
				m_pSegment = reinterpret_cast<Segment<T>*>(GLOBAL_FILLED_SEGMENT_POOL.AcquireObject());
			}

			ASSERT(m_pFilledSegment == nullptr);
			ASSERT(m_pSegment != nullptr);
		}

		T* pData = m_pSegment->m_Objects.back();
		m_pSegment->m_Objects.pop_back();

		return pData;
	}

	void ReleaseObject(T* pData)
	{
		ASSERT(m_pSegment != nullptr);

		if (m_pSegment->m_Objects.size() >= Segment<T>::SEGMENT_SIZE)
		{
			if (m_pFilledSegment != nullptr)
			{
				// global 풀로 반환한다.
				GLOBAL_FILLED_SEGMENT_POOL.ReleaseObject(reinterpret_cast<FullSegment<T>*>(m_pFilledSegment));
				m_pFilledSegment = nullptr;
			}

			ASSERT(m_pFilledSegment == nullptr)

				std::swap(m_pFilledSegment, m_pSegment);
			ASSERT(m_pSegment = nullptr);
			// 현재 Segment가 비어있다 -> 채워진 세그먼트에서 가져온다.

			if (m_pEmptySegment != nullptr)
			{
				std::swap(m_pSegment, m_pEmptySegment);
			}
			else
			{
				// 빈 세그먼트도 없다 -> 새로 할당한다.
				m_pSegment = GLOBAL_EMPTY_SEGMENT_POOL.AcquireObject();
			}
			ASSERT(m_pEmptySegment == nullptr);
			ASSERT(m_pSegment != nullptr);
		}

		ASSERT(m_pSegment->m_Objects.empty());

		m_pSegment->m_Objects.push_back(pData);
	}

	// size가 SEGMENT_SIZE 인 Segment Pool
	static CObjectPool<FullSegment<T>> GLOBAL_FILLED_SEGMENT_POOL;
	// size가 0 인 Segment Pool
	static CObjectPool<Segment<T>> GLOBAL_EMPTY_SEGMENT_POOL;
};

// Fill 된 케이스는 초기에 없다.
template<typename T>
static CObjectPool<FullSegment<T>> GLOBAL_FILLED_SEGMENT_POOL = CObjectPool<FullSegment<T>>(16);

template<typename T>
static CObjectPool<Segment<T>> GLOBAL_EMPTY_SEGMENT_POOL = CObjectPool<Segment<T>>(2);

template<typename T>
class CTLSObjectPool
{
public:
	CTLSObjectPool() = default;
	~CTLSObjectPool() = default;

	CTLSObjectPool(const CTLSObjectPool&) = delete;
	CTLSObjectPool(CTLSObjectPool&&) = delete;

	T* AcquireObject()
	{
		T* obj = m_TLSData.AcquireObject();
		return obj;
	}

	void ReleaseObject(T* pData)
	{
		ASSERT(pData != nullptr);
		m_TLSData.ReleaseObject(pData);
	}


private:
	static thread_local CThreadLocalData<T> m_TLSData;
};

