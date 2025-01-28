
#include "IocpEventTask.h"
#include "Iocp.h"
#include "PacketIOManager.h"

CIocpEventTask::CIocpEventTask()	:
	m_IocpEvent(nullptr)
{
}

CIocpEventTask::CIocpEventTask(OVERLAPPED_ENTRY* pIocpEvent)	:
	m_IocpEvent(pIocpEvent)
{
}

CIocpEventTask::~CIocpEventTask()
{
}

void CIocpEventTask::RunTask()
{
	assert(m_IocpEvent != nullptr);
	CPacketIOManager::GetInst()->ProcessIocpEvent(*m_IocpEvent);
}
