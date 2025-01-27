#include "stdafx.h"
#include "Iocp.h"
#include "Socket.h"
#include "Exception.h"


CIocp::CIocp()
{
	//0개로 생성시 자동으로 프로세서 수 만큼 생성
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);

	m_ThreadCount = SysInfo.dwNumberOfProcessors;
	m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}

CIocp::CIocp(int threadCount)
{
	m_ThreadCount = threadCount;
	m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, threadCount);
}

CIocp::~CIocp()
{
	CloseHandle(m_hIocp);
}

// IOCP에 socket 추가
void CIocp::Add(CSocket& socket, void* userPtr)
{
	if (!CreateIoCompletionPort((HANDLE)socket.m_SocketHandle, m_hIocp, (ULONG_PTR)userPtr, m_ThreadCount))
		throw Exception("IOCP add Socket failed!");
}

void CIocp::Wait(IocpEvents &output, int timeoutMs)
{
	 BOOL r = GetQueuedCompletionStatusEx(m_hIocp, output.m_ArrEvents, MaxEventCount, (ULONG*)&output.m_EventCount, timeoutMs, FALSE);
	 if (!r)
	 {
		 output.m_EventCount = 0;
	 }
}
