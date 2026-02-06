#include "pch.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

class IJob
{
public:
	virtual ~IJob() = default;
	virtual void Execute() = 0;
};


/*
* 원시적인 방법 일감마다 class를 만들어야 함
*/
class HealJob : public IJob
{
public:
	void Execute() override
	{
		std::cout << "Executing Heal Job" << std::endl;
	}

public:
	LONGLONG _target = 0;
	int _healValue = 0;

};

//
//using JobRef = std::shared_ptr<IJob>;
//
//class JobQueue
//{
//public:
//	void PushJob(const JobRef item)
//	{
//		std::lock_guard<std::mutex> lock(_mutex);
//		_items.push(item);
//		//_cv.notify_one();
//	}
//
//	JobRef PopJob()
//	{
//		std::lock_guard<std::mutex> lock(_mutex);
//		if (_items.empty())
//			return nullptr;
//
//		JobRef ret = _items.front();
//		_items.pop();
//		return ret;
//	}
//private:
//	std::queue<JobRef> _items;
//	std::mutex _mutex;
//	//std::condition_variable _cv;
//};


class Room;
class Player;
extern Room GRoom;

class EnterJob : public IJob
{
public:
	EnterJob(Room& room, Player& player)
		: _room(room), _player(player)
	{
	}
	virtual void Execute() override
	{
		_room.Enter(_player);
	}
public:
	Room& _room;
	Player& _player;
};

class SendBufferRef;

class Room
{
	friend class EnterJob;
	friend class LeaveJob;
	friend class BroadcastJob;
private:
	// 싱글스레드 환경인것마냥 코딩
	void Enter(Player& player);
	void Leave(Player& player);
	void Broadcast(SendBufferRef& sendbuffer);

public:
	// 멀티스레드 환경에서는 일감으로 접근
	void PushJob(JobRef job)
	{
		_jobQueue.PushJob(job);
	}

	void FlushJobs()
	{
		while (true)
		{
			JobRef job = _jobQueue.PopJob();
			if (job == nullptr)
				break;
			job->Execute();
		}
	}

	// 2세대 방식
	template<typename T, typename Ret, typename... Args>
	void PushJob(T* obj, Ret(T::* memFunc)(Args...), Args... args)
	{
		auto job = std::make_shared<MemberJob<T, Ret, Args...>>(static_cast<T*>(this), memFunc, args...);
		_jobQueue.PushJob(job);
	}
	// 호출 GRoom.PushJob(&Room::Enter, player);



private:
	JobQueue _jobQueue;
	std::vector<Player*> _players;
};


class EnterJob : public IJob
{
public:
	EnterJob(Room& room, Player& player)
		: _room(room), _player(player)
	{
	}
	virtual void Execute() override
	{
		_room.Enter(_player);
	}
public:
	Room& _room;
	Player& _player;
};



class LeaveJob : public IJob
{
public:
	LeaveJob(Room& room, Player& player)
		: _room(room), _player(player)
	{
	}
	virtual void Execute() override
	{
		_room.Leave(_player);
	}
public:
	Room& _room;
	Player& _player;
};




class BroadcastJob : public IJob
{
public:
	BroadcastJob(Room& room, SendBufferRef& sendbuffer)
		: _room(room), sendbuffer(sendbuffer)
	{
	}
	virtual void Execute() override
	{
		_room.Broadcast(sendbuffer);
	}
public:
	Room& _room;
	SendBufferRef& sendbuffer;
};

//1세대 방식
//////////////////////////////////////////////////////////////////////


// 함수자  functor
template<typename Ret, typename... Args>
class FuncJob : pulbic IJob
{
	using FuncType = Ret(*)(Args...);
public:
	FuncJob(FuncType func, Args... args) : _func(func), _tuple(args...)
	{
	}

	virtual void Execute(Args... args) override
	{
		xapply_impl(_func, _tuple);
	}

private:
	FuncType _func;
	std::tuple<Args...> _tuple;

};


// c++11 apply

template<int... Remains >
struct seq
{
};

template<int N, int... Remains >
struct gen_seq : gen_seq<N - 1, N - 1, Remains...>
{
};

template<typename Ret, typename... Args>
void xapply_impl(Ret(*func)(Args...), std::tuple<Args...>& t, seq<>)
{
	xapply_helper(func, seq<sizeof...(Args)>(), t);
}


//helper
template<typename F, typename... Args, int... I>
void xapply_helper(F func, seq<I...>, std::tuple < Args... >& tup)
{
	func(std::get<I>(t)...);
}


// class 멤버함수 호출

template<typename T, typename Ret, typename... Args>
void xapply_impl(T* obj, Ret(T::*func)(Args...), std::tuple<Args...>& t, seq<>)
{
	xapply_helper(obj, func, seq<sizeof...(Args)>(), t);
}


//helper
template<typename T,typename F, typename... Args, int... I>
void xapply_helper(T* obj,F func, seq<I...>, std::tuple < Args... >& tup)
{
	(obj->*func)(std::get<I>(t)...);
}


template<typename Ret, typename... Args>
class MemberJob
{
	using FuncType = Ret(T::*)(Args...);
public:
	MemberJob(T* obj, FuncType func, Args,... args) : 
	_obj(obj),
	_func(func),
	_tuple(args)
	{
	}

	Ret Execute()
	{
		xapply_impl(obj, _func, _tuple, _tuple);
	}

private:
	T* obj;
	FuncType _func;
	std::tuple<Args...> _tuple;

};

/*
{
	FuncJob<void, int, int> item(HealByValue, 100, 10);
	item.Execute();

}
{
	Knight k1;
	MemberJob job2(&k1, &Knight::HealByValue, 100, 10);
	job2.Execute();
}

*/



//2세대 방식
//////////////////////////////////////////////////////////////////////
using JobRef = std::shared_ptr<IJob>;

class Job
{
public:
	virtual ~Job() = default;
	virtual void Execute() = 0;
};




class JobQueue
{
public:
	void PushJob(const JobRef job)
	{
	//write lock
		std::lock_guard<std::mutex> lock(_mutex);
		_items.push(job);
		//_cv.notify_one();
	}

	JobRef PopJob()
	{
		//write lock
		std::lock_guard<std::mutex> lock(_mutex);
		if (_items.empty())
			return nullptr;

		JobRef ret = _items.front();
		_items.pop();
		return ret;
	}
private:
	std::queue<JobRef> _items;
	std::mutex _mutex;
	//std::condition_variable _cv;
};

#include <functional>

void HelloWorld(int a, int b)
{
	std::cout << "Hello, World!" << std::endl;
}

void test()
{
	std::shared_ptr<Player> player = std::make_shared<Player>();
	
	//클래스
	std::function<void()> func = [=]() 
		{// 모든 대상을 복사, &는 참조
		/*
		* std::function<void()> func = [&player]() 
		{// 모든 대상을 복사, &는 참조
		&player :: player만  참조
		
		*/

		GRoom.Enter(*player);
		
		
		// HelloWorld (1, 2);
		// 람다에서 이 argument를 캡쳐해서 사용
		// 클로저 컴파일러 내부에서 class를 만들어 사용
	};
}


class Knight : public std::enable_shared_from_this<Knight>
{
public:
	void HealMe(int healValue)
	{
		std::cout << "Healing " << " " << healValue << std::endl;
	}

	void Test()
	{
		//멤버함수 포인터
		/*auto item = [&]()
			{
				HealMe(_hp);
			};*/

		// 위 케이스는 실제로 아래와 같이 this를 캡처함
		// this에 대한 포인터를 복사해 댕글링 가능성 존재
		/*auto item = [this]()
			{
				HealMe(this->_hp);
			};*/
		auto job = [self = std::enable_shared_from_this<Knight>::shared_from_this()]()
		{
			self->HealMe(self->_hp);
		};
		
	}





private
	int _hp = 100;
};



/*
* --------------------------
* Job
* --------------------------
*/

using CallbackType = std::function<void()>;


class Job
{
public:
	Job(CallbackType&& callback)
		: _callback(std::move(callback))
	{
	}


	template<typename T, typename Ret, typename... Args>
	Job(std::shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
		:
	{
		_callback = [owner, memFunc, args...]() {
			(owner.get()->*memFunc)(args...);
		};
	}

	/*
	* 
	
	*/


	void Execute()
	{
		_callback();
	}

private:
	CallbackType _callback;
};


//JobSerializer.h
class JobSerializer : public std::enable_shared_from_this<JobSerializer>
{
public:
	JobSerializer();
	~JobSerializer();

	void PushJob(CallbackType&& callback)
	{
		auto job = ObjectPool<Job>::MakeShare(std::move(callback));
		_jobQueue.PushJob(job);
	} 

	template<typename T, typename Ret, typename... Args>
	void PushJob(Ret(T::* memFunc)(Args...), Args&&... args)
	{
		std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
		auto job = ObjectPool<Job>::MakeShare(owner, memFunc, std::forward<Args>(args)...);
		_jobQueue.PushJob(job);
	}

	virtual void FlushJobs() abstract;


protected:
	JobQueue _jobQueue;
};

JobSerializer::JobSerializer()
{
}

JobSerializer::~JobSerializer()
{
}


class Room : public JobSerializer
{
	friend class EnterJob;
	friend class LeaveJob;
	friend class BroadcastJob;
private:
	// 싱글스레드 환경인것마냥 코딩
	void Enter(Player& player);
	void Leave(Player& player);
	void Broadcast(SendBufferRef& sendbuffer);

public:
	// 멀티스레드 환경에서는 일감으로 접근

	virtual void FlushJobs() override
	{
		while (true)
		{
			JobRef job = _jobQueue.PopJob();
			if (job == nullptr)
				break;
			job->Execute();
		}
	}


private:
	std::vector<Player*> _players;
};

/*
사용 : GRoom->PushJob(&Room::Enter, player); 
PushJob / Flush의 스레드 분배에 대한 문제가 존재
*/ 





/*
* JobQueue에서 shared_ptr은 순환참조를 유발하기에 이를 의도적으로 끊어줄 필요가 존재 따라서 
* pushjob 내부에서 weak_ptr로 들고있게 하던가 
* clearJob등을 구현하여 사용하는 등의 방식 사용 가능
* 
* 
* 
* JobQueue를 어떻게 배치하느냐는 구조마다 다르나
* 심리스 기반의 경우 액터 단위로 다 배치하는 경우도 존재 
* FlushJob 호출이 몇십만개가 될 수도있음
* 
* 이때 50만개를 일일이 무한루프로 계쏙 뺑뻉이돌면서 실행하는것도 좀 애매함.
* 일감이 없는데도 굳이 매번마다 무한루프를 하면서 체크하는것도 굉장히 미련함
* 
* condition variable 같은거 이용해서 일감이 있을떄만 50만개를 넘어가면 조건변수로 배치하는것도 좀 말이 안 됨
* 
* 
* 정책이 많이 갈리는 편
* 푸시 잡을 같이 실행까지 담당하는 것이 좋아보임
*/

// 네이밍 변경 JobQueue -> 공용 template LockQueue로 변경




template<typename T>
class LockQueue
{
public:
	void Push(T item)
	{
		//write lock
		std::lock_guard<std::mutex> lock(_mutex);
		_items.push(item);
		//_cv.notify_one();
	}

	T Pop()
	{
		//write lock
		std::lock_guard<std::mutex> lock(_mutex);
		if (_items.empty())
			return nullptr;

		JobRef ret = _items.front();
		_items.pop();
		return ret;
	}

	void Clear()
	{
		//write lock
		_items.clear();

		//_items = std::queue<T>();

	}

	void PopAll(OUT std::vector<T>& items)
	{
		//write lock
		std::lock_guard<std::mutex> lock(_mutex);

		items.reserve(_items.size());
		while (T item = Pop())
		{
			items.push_back(item);
		}
	}

private:
	std::queue<T> _items;
	std::mutex _mutex;
	//std::condition_variable _cv;
};


// 추가적으로 기존 JobSerializer 를 JobQueue로 다시 네이밍을 변경


//JobQueue.h
class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
	JobQueue();
	~JobQueue();

	// PushJob네이밍은 job만 넣고 끝나는 느낌이니 명칭 면경 DoAsync
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShare(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args&&... args)
	{
		std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShare(owner, memFunc, std::forward<Args>(args)...));
	}


	void DoTimer(unsigned long long tickAfter, CallbackType&& callback)
	{
		JobRef job = ObjectPool<Job>::MakeShare(std::move(callback)); 
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void DoTimer(Ret(T::* memFunc)(Args...), Args&&... args)
	{
 		 std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
		 Push(ObjectPool<Job>::MakeShare(owner, memFunc, std::forward<Args>(args)...));
	}



	void ClearJobs() {_jobs.Clear(); }

	virtual void FlushJobs() abstract;
	void Push(JobRef&& job)
	{
	// jobcount에 대한 덧셈뺄셋은 순서가 바뀌면 안됨
		const int prevCount = _jobCount.fetch_add(1);

		// 첫번째 job을 넣은 쓰레드가 실행까지 담당
		if (prevCount == 0)
		{
			Execute();
		}

	}
	void Execute()
	{
		while (true)
		{
			std::vector<JobRef> jobs;
			_jobs.PopAll(OUT jobs);

			const int jobCount = static_cast<int>(jobs.size());
			for (int i = 0; i < jobCount; ++i)
			{
				jobs[i]->Execute();
			}

			if (_jobCount.fetch_sub(jobCount) == jobCount)
			{
				return ;
			}
		}

	}
private:


protected:
	LockQueue<JobRef> _jobs;
	std::atomic<int> _jobCount = 0;
};

JobQueue::JobQueue()
{
}

JobQueue::~JobQueue()
{
}

/* 
* 해당 방식의 문제점
* 
* 일감이 계속 들어오면 무한히 들어오는 문제가 발생. 
* 
* 한쪽에만 몰린다면 특정 부분에만 렉이 걸리던가하는 문제 발생
* 특히 처음 시작한 스레드에서 해당 액터에서 다른액터로 요청 보냄 -> 여기서도 현재 스레드 기준으로 돌아가게됨
* -> 타고타고 액터 작동이 한 스레드로 몰리는 문제 발생.
*/

///////////////////////////////////////////////////////////////////////////////////////////
// 개선
using JobQueueRef = std::shared_ptr<JobQueue>;

class GlobalQueue
{
public:
	GlobalQueue();
	~GlobalQueue();

	void Push(JobQueueRef jobQueue)
	{
		_jobQueues.Push(jobQueue);
	}

	JobQueueRef Pop()
	{
		return _jobQueues.Pop();
	}


private:
	LockQueue<JobQueueRef> _jobQueues;

};

GlobalQueue* GGlobalQueue = new GlobalQueue();

//
thread_local JobQueue *LCurrentJobQueue;


class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
	JobQueue() = default;
	~JobQueue() = default;

	// PushJob네이밍은 job만 넣고 끝나는 느낌이니 명칭 면경 DoAsync
	void DoAsync(CallbackType&& callback)
	{
		Push(ObjectPool<Job>::MakeShare(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args&&... args)
	{
		std::shared_ptr<T> owner = std::static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShare(owner, memFunc, std::forward<Args>(args)...));
	}


	void ClearJobs() { _jobs.Clear(); }

	virtual void FlushJobs() abstract;
private:
	void Push(JobRef&& job)
	{
		// jobcount에 대한 덧셈뺄셋은 순서가 바뀌면 안됨
		const int prevCount = _jobCount.fetch_add(1);

		// 첫번째 job을 넣은 쓰레드가 실행까지 담당
		if (prevCount == 0)
		{
			if (LCurrentJobQueue != nullptr)
			{
				Execute();
			}
			else
			{
				GGlobalQueue->Push(shared_from_this());
			}
		}

	}
	void Execute()
	{
		LCurrentJobQueue = this;
		while (true)
		{
			std::vector<JobRef> jobs;
			_jobs.PopAll(OUT jobs);

			const int jobCount = static_cast<int>(jobs.size());
			for (int i = 0; i < jobCount; ++i)
			{
				jobs[i]->Execute();
			}

			if (_jobCount.fetch_sub(jobCount) == jobCount)
			{
				LCurrentJobQueue = nullptr;
				return;
			}

			const LONGLONG now = ::GetTickCount64();
			if (now > LEndTickCount)
			{
				LCurrentJobQueue = nullptr;

				// 여기도달하기전에 job stealing 고려
				GGlobalQueue->Push(shared_from_this());
				return;
			}
		}

	}


protected:
	LockQueue<JobRef> _jobs;
	std::atomic<int> _jobCount = 0;
};


thread_local LONGLONG LEndTickCount = 0;

void DoGlobalQueueWork()
{
	while (true)
	{
		LONGLONG now = ::GetTickCount64();
		if (now > LEndTickCount)
		{
			break;
		}

		JobQueueRef jobQueue = GGlobalQueue->Pop();
		if (jobQueue == nullptr) //assert
			break;
		
		jobQueue->Execute();
	}
}

enum { WORKER_TICK = 64 };
// 이후 자동으로 계산되도록 이후 보정

void DoWorkerJob()
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK; 

		// IOCP DISPATCH Timeout 설정

		// 자체 처리 EventLoop도 돌아야함
		DoGlobalQueueWork();

		/*
		* 결국 스레드 갯수를 역할별로 분배할경우 
		* 결국에 어느단에서 병목이 생길지는 알 수 없으니
		* 전체 스레드가 만능이 되자 주의
		*/

		// case : 게임컨텐츠로직도 iocp 스레드에서 모두 처리하도록 확장 가능




		// 개인적으로 아쉬운 부분 캐시 효율이 떨어지지않을까?
		// 캐시 효율상 루프로 다시 실행되는 액터 주체는 최근 실행한 스레드에서 그대로 실행되도록 하는게 좋지않을까


	}
}

/////////////////////////////////////////

//JobTimer

struct JobData
{
	JobData(std::weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{
	}

	std::weak_ptr<JobQueue> owner;
	JobRef job;
};


struct TimerItem
{
	bool operator>(const TimerItem& other) const
	{
		return executeTick > other.executeTick;
	}

	LONGLONG executeTick = 0;
	JobData *jobData = nullptr; // 생포인터
};


class JobTimer
{
public:
	void Reserve(unsigned long long tickAfter, std::weak_ptr<JobQueue> owner, JobRef job)
	{
		TimerItem* item = new TimerItem();
		item->executeTick = ::GetTickCount64() + tickAfter;

		//ObjectPoo;::<JobData>::Pop(owner, job);
		item->jobData = new JobData(owner, job);
	
	
		std::lock_guard<std::mutex> lock(_mutex);
		_items.push(item);
	}


	void Distribute(unsigned long long maxCount)
	{
		if (_distributing.exchange(true) == true)
		{
			return;
		}

		std::vector <TimerItem*> itemsToDistribute;

		LONGLONG now = ::GetTickCount64();
		unsigned long long distributedCount = 0;
		while (distributedCount < maxCount)
		{
			TimerItem* item = nullptr;
			{
				std::lock_guard<std::mutex> lock(_mutex);
				if (_items.empty())
					break;
				item = _items.top();

				itemsToDistribute.push_back(item);
				 
				_items.pop();
			}
			if (item != nullptr)
			{
				std::shared_ptr<JobQueue> owner = item->jobData->owner.lock();
				if (owner != nullptr)
				{
					owner->Push(std::move(item->jobData->job));
				}
				delete item->jobData;
				delete item;
				++distributedCount;
			}
		}


		for (TimerItem* item : itemsToDistribute)
		{
			if (JobQueueRef owner = item->jobData->owner.lock)

				owner->Push(std::move(item->jobData->job));

			//ObjectPool<JobData>::Push(item->jobData);
			}
		}
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		while (!_items.empty())
		{
			TimerItem* item = _items.top();
			_items.pop();
			delete item->jobData;
			delete item;
		}
	}
private:
	
	 std::priority_queue<TimerItem*, std::vector<TimerItem*>, std::greater<TimerItem*>> _items;
	std::mutex _mutex;
	std::atomic<bool> _distributing = false;
};

void ThreadManager_ DistributeJobTimer()
{
	const unsigned long long now = GetTickCount64();



	GJobTimer->Distribute(100);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));



	
}


class SpinLock
{
public:
	void Lock()
	{
		while (_flag.test_and_set(std::memory_order_acquire))
		{
			//pause
			this_thread::yield();
			//_flag.wait(true, std::memory_order_relaxed);
		}
	}
	void Unlock()
	{
		_flag.clear(std::memory_order_release);
	}
private:
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
}
