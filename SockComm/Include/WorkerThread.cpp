#include "WorkerThread.h"

template<typename Ret, typename ...Args>
inline CWorkerThread<Ret, Args...>::CWorkerThread()
{
	m_Thread
}

template<typename Ret, typename ...Args>
inline CWorkerThread<Ret, Args...>::~CWorkerThread()
{
}

template<typename Ret, typename ...Args>
void CWorkerThread<Ret, Args...>::Run()
{
}

template<typename Ret, typename ...Args>
const int CWorkerThread<Ret, Args...>::GetWorkCount()
{
	return m_QueueMutex.size();
}

template<typename Ret, typename ...Args>
void CWorkerThread<Ret, Args...>::AddTask(std::function<Ret(Args...)> Task)
{

}
