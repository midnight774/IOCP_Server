#pragma once

class CSocket;
struct IocpEvents;

// I/O Completion Port 객체.
class CIocp
{
public:
	// 1회의 GetQueuedCompletionStatus이 최대한 꺼내올 수 있는 일의 갯수
	static const int MaxEventCount = 1000;

	CIocp();
	CIocp(int ThreadCount);
	~CIocp();

	void Add(CSocket& socket, void* userPtr);
	
	HANDLE m_hIocp;
	int m_ThreadCount;
	void Wait(IocpEvents &output, int timeoutMs);
};

// IOCP의I/O 완료신호들
struct IocpEvents
{
public:
	//이벤트 구조체
	OVERLAPPED_ENTRY m_ArrEvents[CIocp::MaxEventCount];
	int m_EventCount;
};


