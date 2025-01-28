
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

public:
	void Run();
	
public:
	inline const size_t GetTaskCount() const
	{
		return m_qTask.size();
	}

	inline void PushTask(std::shared_ptr<CBaseTask> pTask)
	{
		m_qTask.push(pTask);
	}
};

