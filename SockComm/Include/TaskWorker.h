
#pragma once
#include "SockComm.h"

class CBaseTask;

class CTaskWorker
{
public:
	CTaskWorker();
	~CTaskWorker();

protected:
	std::shared_ptr<std::thread>			m_Thread;
	std::mutex								m_Mtx;
	std::queue<std::shared_ptr<CBaseTask>>	m_qTask;
	std::condition_variable					m_TaskAvailable;
	bool									m_IsStop;

public:
	void Run();
	
	
public:
	const size_t GetTaskCount();
	void PushTask(std::shared_ptr<CBaseTask> pTask);
	void StopThread();
	
	
	void ThreadJoin()
	{
		if (m_Thread->joinable())
			m_Thread->join();
	}
};

