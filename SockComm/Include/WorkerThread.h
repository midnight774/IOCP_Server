
#pragma once

#include <mutex>
#include <queue>
#include <functional>

template<typename Ret, typename... Args>
class CWorkerThread
{
public:
	CWorkerThread();
	~CWorkerThread();

private:
	std::thread m_Thread;
	std::mutex m_QueueMutex;
	std::queue<std::function<Ret(Args...)>> m_qTask;
	
public:
	void Run();
	const int GetWorkCount();
	void AddTask(std::function<Ret(Args...)> Task);

};
