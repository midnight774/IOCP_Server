
#include "IocpEventTask.h"
#include "Iocp.h"
#include "PacketIOManager.h"

CIocpEventTask::CIocpEventTask()	:
	m_IocpEvent()
{
}

CIocpEventTask::CIocpEventTask(OVERLAPPED_ENTRY IocpEvent)	:
	m_IocpEvent(IocpEvent)
{
}

CIocpEventTask::~CIocpEventTask()
{
}

void CIocpEventTask::RunTask()
{
	CPacketIOManager::GetInst()->ProcessIocpEvent(m_IocpEvent);
}
