#pragma once
#include<thread>
#include<map>
#include<mutex>
#include<shared_mutex>
#include<future>
#include<queue>
#include<functional>
#include<chrono>

using std::map;
using std::future;
using std::condition_variable_any;
using std::queue;
using std::shared_mutex;
using std::recursive_mutex;
using std::function;
using std::pair;
using std::shared_future;
using std::shared_lock;
using std::async;
using std::launch;
using std::lock_guard;
using std::unique_lock;
using std::future_status;
using std::make_pair;
using std::this_thread::get_id;
using std::chrono::milliseconds;
using std::chrono::seconds;

class AllenThreadPool
{
#define DEBUG_INFO
public:
	enum ThreadMsg {
		TM_None,
		TM_JOB,
		TM_WORKING,
		TM_IDLE,
		TM_INIT_WORKER,
		TM_EXIT,
		TM_TERMINAL
	};

#define MSG_QUE_TYPE pair<decltype(get_id()), ThreadMsg>
#define WORKER_LIST_TYPE decltype(get_id()), shared_future<void>

	map<WORKER_LIST_TYPE> m_WorkersList;
	future<void> m_BackgroundTask;
	queue<function<void()>> m_JobList;
	shared_mutex m_EventLocker;
	recursive_mutex m_SrcLocker;
	condition_variable_any m_cvDispatchJob;
	condition_variable_any m_cvWorkerEvent;
	condition_variable_any m_cvWorkerInit;
	condition_variable_any m_cvJobFinished;
	queue<MSG_QUE_TYPE> m_ThreadMsg;
	size_t m_LimitWorkersSize;
	size_t m_sNum2Exit;
	size_t m_sAssignedTask;
	size_t m_sProccessTask;
	size_t m_sFinishedTask;

private:

	size_t InitWorkers(size_t num)
	{
		if (num > m_LimitWorkersSize) num = m_LimitWorkersSize;
		if (m_WorkersList.size() == num) return m_WorkersList.size();
		if (m_WorkersList.size() < num)
		{
			// add workers
			while (m_WorkersList.size() < num)
			{
				shared_lock<shared_mutex> ul(this->m_EventLocker);
				future<void> tmp;
				tmp = async(launch::async, [this, &tmp]() {
					{
						lock_guard<recursive_mutex> lg(this->m_SrcLocker);
						m_WorkersList[get_id()] = tmp.share();
						//cout << "new worker created with id " << this_thread::get_id() << endl;
						this->m_cvWorkerInit.notify_one();
					}
					shared_lock<shared_mutex> lg(this->m_EventLocker);

					while (true)
					{
						function<void()> job;
						{
							lock_guard<recursive_mutex> lg(this->m_SrcLocker);
							if (m_sNum2Exit > 0)
							{
								m_sNum2Exit--;
								this->m_ThreadMsg.push(make_pair(get_id(), ThreadMsg::TM_EXIT));
								this->m_cvWorkerEvent.notify_one();
								break;
							}
							if (m_JobList.size() > 0)
							{
								job = m_JobList.front();
								m_JobList.pop();
								m_sProccessTask++;
							}
						}
						if (job)
						{
							job();
							lock_guard<recursive_mutex> lg(this->m_SrcLocker);
							m_sProccessTask--;
							m_sFinishedTask++;
							if (m_sFinishedTask == m_sAssignedTask) m_cvJobFinished.notify_all();
						}
						else
							this->m_cvDispatchJob.wait(lg);
					}
				});
				this->m_cvWorkerInit.wait(ul);
				//cout << "workers size:" << m_WorkersList.size() << endl;
			}
		}
		else
		{
			// remove workers
			lock_guard<recursive_mutex> lg(this->m_SrcLocker);
			m_sNum2Exit = m_WorkersList.size() - num;
			//cout << "set exit " << m_sNum2Exit << endl;
			m_cvDispatchJob.notify_all();
		}
		return m_WorkersList.size();
	}

	void init()
	{
		m_LimitWorkersSize = 8;
		m_sNum2Exit = 0;
		m_BackgroundTask = async(launch::async, [this]() {
			unique_lock<shared_mutex> ul(this->m_EventLocker);
			bool isExit = false;
			while (true)
			{
				MSG_QUE_TYPE* tmp = nullptr;
				{
					lock_guard<recursive_mutex> lg(this->m_SrcLocker);
					if (m_ThreadMsg.size() > 0)
					{
						tmp = new MSG_QUE_TYPE(m_ThreadMsg.front());
						m_ThreadMsg.pop();
					}
				}
				if (!tmp)
					this->m_cvWorkerEvent.wait(ul);
				else
				{
					switch (tmp->second)
					{
					case ThreadMsg::TM_TERMINAL:
					{
						InitWorkers(0);
						isExit = true;
						{
							lock_guard<recursive_mutex> lg(this->m_SrcLocker);
							if (m_WorkersList.size() == 0)
								return;
						}
						//cout << "signal for exit msg monitor is received" << endl;
					}break;
					case ThreadMsg::TM_EXIT:
					{
						lock_guard<recursive_mutex> lg(this->m_SrcLocker);
						auto ret = m_WorkersList[tmp->first].wait_for(seconds(3));
						if (ret != future_status::ready)
						{
							//cout << "thread exit failed" << endl;// maybe needs some native API to terminal thread
						}
						else
						{
							m_WorkersList.erase(tmp->first);
							//cout << "worker exit and workers left " << m_WorkersList.size() << endl;
						}
						if (m_WorkersList.size() == 0 && isExit)
							return;
					}break;
					}
					if (tmp) delete tmp;
				}
			}
		});
	}

public:
	AllenThreadPool()
	{
		this->init();
	}

	AllenThreadPool(size_t WorkerNumbers)
	{
		this->init();
		this->SetWorkers(WorkerNumbers);
	}

	~AllenThreadPool()
	{
		{
			lock_guard<recursive_mutex> lg(this->m_SrcLocker);
			m_ThreadMsg.push(make_pair(get_id(), ThreadMsg::TM_TERMINAL));
		}
		m_cvWorkerEvent.notify_one();
		m_BackgroundTask.wait();
	}

	void AddJob(function<void()> job)
	{
		lock_guard<recursive_mutex> lg(this->m_SrcLocker);
		m_JobList.push(job);
		m_sAssignedTask++;
		m_cvDispatchJob.notify_all();
	}

	void AddJob(function<void()> job, int NumbersOfJobs)
	{
		if (NumbersOfJobs <= 0) return;
		lock_guard<recursive_mutex> lg(this->m_SrcLocker);
		m_sAssignedTask += NumbersOfJobs;
		while (--NumbersOfJobs >= 0)
			m_JobList.push(job);
		m_cvDispatchJob.notify_all();
	}

	void SetWorkers(size_t WorkerNumbers)
	{
		InitWorkers(WorkerNumbers);
	}

	void Wait()
	{
		{
			lock_guard<recursive_mutex> lg(this->m_SrcLocker);
			if (m_sAssignedTask == m_sFinishedTask)
				return;
		}
		shared_lock<shared_mutex> lg(this->m_EventLocker);
		m_cvJobFinished.wait(lg);
	}

#undef MSG_QUE_TYPE
#undef WORKER_LIST_TYPE
};