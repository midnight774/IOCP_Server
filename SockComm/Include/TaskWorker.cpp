
#include "TaskWorker.h"
#include "BaseTask.h"

CTaskWorker::CTaskWorker()	:
	m_IsStop(false)
{
	m_Thread = std::make_shared<std::thread>(&CTaskWorker::Run, this);
}

CTaskWorker::~CTaskWorker()
{
	if (!m_IsStop)
	{
		StopThread();

		if (m_Thread->joinable())
			m_Thread->join();
	}
}

void CTaskWorker::Run()
{
	while (!m_IsStop)
	{
		std::shared_ptr<CBaseTask> CurTask;

		//Task¸¦ ²¨³»¿Â´Ù.
		{
			std::lock_guard<std::mutex> Lock(m_Mtx);

			if (!m_qTask.empty())
			{
				CurTask = m_qTask.front();
				m_qTask.pop();
			}
		}

		if (CurTask)
			CurTask->RunTask();

		else
		{
			std::unique_lock<std::mutex> Lock(m_Mtx);
			m_TaskAvailable.wait(Lock, [this]() {return m_IsStop || !m_qTask.empty(); });
		}
	}
}

const size_t CTaskWorker::GetTaskCount()
{
	std::lock_guard<std::mutex> Lock(m_Mtx);
	return m_qTask.size();
}

void CTaskWorker::PushTask(std::shared_ptr<CBaseTask> pTask)
{
	std::lock_guard<std::mutex> Lock(m_Mtx);
	m_qTask.push(pTask);
	m_TaskAvailable.notify_all();
}

void CTaskWorker::StopThread()
{
	std::lock_guard<std::mutex> Lock(m_Mtx);
	m_IsStop = true;
	m_TaskAvailable.notify_all();
}
