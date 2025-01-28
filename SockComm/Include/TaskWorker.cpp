
#include "TaskWorker.h"
#include "BaseTask.h"

CTaskWorker::CTaskWorker()
{
	m_Thread = std::make_shared<std::thread>(&CTaskWorker::Run, this);

	if (m_Thread->joinable())
		m_Thread->join();
}

CTaskWorker::~CTaskWorker()
{
}

void CTaskWorker::Run()
{
	while (true)
	{
		//100ms 대기후 큐에 접근한다.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		//추가된 Task 처리
		std::lock_guard<std::mutex> Lock(m_Mtx);
		while (!m_qTask.empty())
		{
			std::shared_ptr<CBaseTask> CurTask = m_qTask.front();
			m_qTask.pop();

			CurTask->RunTask();
		}
	}
}
