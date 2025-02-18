#include "PacketIOManager.h"
#include "RemoteClient.h"
#include "SessionManager.h"
#include "TaskWorker.h"
#include "IocpEventTask.h"

DEFINITION_SINGLETON(CPacketIOManager);

CPacketIOManager::CPacketIOManager()	:
	m_ListenSocket(SocketType::Tcp),
	m_IsStopWorking(false)
{
	m_ListenSocket.Bind(CEndpoint("0.0.0.0", 55555));

}

CPacketIOManager::~CPacketIOManager()
{
	for (int i = 0; i < m_vecThreadWorker.size(); ++i)
	{
		m_vecThreadWorker[i]->StopThread();
		m_vecThreadWorker[i]->ThreadJoin();
	}

	m_ListenSocket.Close();

	// 백그라운드 진행 중인 Overlapped I/O 완료를 모두 체크 후 종료
	while(m_ListenSocket.m_isReadOverlapped)
	{
		IocpEvents readEvents;
		m_Iocp.Wait(readEvents, 100);

		// 받은 이벤트 각각을 처리
		for (int i = 0; i < readEvents.m_EventCount; i++)
		{
			auto& readEvent = readEvents.m_ArrEvents[i];
			if (readEvent.lpCompletionKey == 0) // 리슨소켓이면
			{
				m_ListenSocket.m_isReadOverlapped = false;
			}
			else
			{
				std::shared_ptr<CRemoteClient> RemoteClient = CSessionManager::GetInst()->FindClient((CRemoteClient*)readEvent.lpCompletionKey);
				if (RemoteClient)
				{
					RemoteClient->m_TcpSocket.m_isReadOverlapped = false;
				}
			}
		}
	}

}

bool CPacketIOManager::Init()
{
	m_ListenSocket.Listen();
	m_Iocp.Add(m_ListenSocket, nullptr);

	//Overlapped Accept
	m_PendingClient = std::make_shared<CRemoteClient>(SocketType::Tcp);

	std::string ErrorText;
	if (!m_ListenSocket.AcceptOverlapped(m_PendingClient->m_TcpSocket, ErrorText)
		&& WSAGetLastError() != ERROR_IO_PENDING)
	{
		throw Exception("Overlapped AcceptEx Failed");
	}

	m_ListenSocket.m_isReadOverlapped = true;

	m_ThreadCnt = 4;

	if (m_ThreadCnt == 0)
	{
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		SysInfo.dwNumberOfProcessors;
		InitThreadPool(SysInfo.dwNumberOfProcessors);
	}

	else
		InitThreadPool(m_ThreadCnt);

	return true;
}

void CPacketIOManager::IocpLoop()
{
	IocpEvents ReadEvents;
	m_Iocp.Wait(ReadEvents, 100);

	int EventCnt = ReadEvents.m_EventCount;
	for (int i = 0; i < EventCnt; ++i)
	{
		//ProcessIocpEvent(ReadEvents.m_ArrEvents[i]);
		std::shared_ptr<CIocpEventTask> Task = std::make_shared<CIocpEventTask>(ReadEvents.m_ArrEvents[i]);

		int Idx = GetLeastWorkThreadIdx();
		m_vecThreadWorker[Idx]->PushTask(Task);
	}
}

void CPacketIOManager::ProcessIocpEvent(const OVERLAPPED_ENTRY& Event)
{
	std::lock_guard<std::mutex> Lock(m_IOMutex);

	CSessionManager* SMInst = CSessionManager::GetInst();

	if (Event.lpCompletionKey == 0)//리슨소켓일 경우
	{
		m_ListenSocket.m_isReadOverlapped = false;
		//AcceptEx가 I/O 완료 상태 되었으니 TCP 연결 완료 처리 해준다.
		if (m_PendingClient->m_TcpSocket.UpdateAcceptContext(m_ListenSocket) != 0)
		{
			//에러가 나면 리슨소켓 닫기
			m_ListenSocket.Close();
		}

		else
		{
			std::shared_ptr<CRemoteClient> RemoteClient = m_PendingClient;

			// 새 TCP 소켓 IOCP에 추가
			m_Iocp.Add(m_PendingClient->m_TcpSocket, RemoteClient.get());

			// I/O 수신 요청을 걸어둔다.
			if (RemoteClient->m_TcpSocket.ReceiveOverlapped() != 0
				&& WSAGetLastError() != ERROR_IO_PENDING)
			{
				// 에러
				RemoteClient->m_TcpSocket.Close();
			}
			else
			{
				//클라이언트 서버 입장 처리
				SMInst->InsertClient(RemoteClient);
			}

			// 계속해서 새 연결 받기 위해 리슨소켓 overlapped I/O 걸기.
			m_PendingClient.reset();
			m_PendingClient = std::make_shared<CRemoteClient>(SocketType::Tcp);
			std::string errorText;
			if (!m_ListenSocket.AcceptOverlapped(m_PendingClient->m_TcpSocket, errorText)
				&& WSAGetLastError() != ERROR_IO_PENDING)
			{
				// 에러나면 리슨소켓 정리
				m_ListenSocket.Close();
			}
			else
			{
				m_ListenSocket.m_isReadOverlapped = true;
			}
		}
	}
	else  // TCP 연결 소켓이면
	{
		std::shared_ptr<CRemoteClient> RemoteClient = SMInst->FindClient((CRemoteClient*)Event.lpCompletionKey);

		if (RemoteClient)
		{
			if (Event.lpOverlapped == &(RemoteClient->m_TcpSocket.m_SendOverlappedStruct)) // 송신 이벤트인 경우
			{
			}

			else if(Event.lpOverlapped == &(RemoteClient->m_TcpSocket.m_ReceiveOverlappedStruct))// 수신 이벤트인 경우
			{
				RemoteClient->SetOverlappedReceiveFlag(false);
				int DataLength = Event.dwNumberOfBytesTransferred;

				if (DataLength <= 0)
				{
					// 읽은 결과가 0 혹은 음수이므로 끝내자
					SMInst->EraseClient(RemoteClient);
				}
				else
				{
					char* EchoData = RemoteClient->m_TcpSocket.m_ReceiveBuffer;
					EchoChattingData(EchoData, DataLength);

					// 다시 수신을 받으려면 overlapped I/O를 걸어야 한다.
					if (RemoteClient->m_TcpSocket.ReceiveOverlapped() != 0
						&& WSAGetLastError() != ERROR_IO_PENDING)
					{
						//오류시 삭제
						SMInst->EraseClient(RemoteClient);
					}
					else
					{
						// I/O를 걸었다. 완료를 대기 상태로 바꾸자.
						RemoteClient->SetOverlappedReceiveFlag(true);
					}
				}
			}
			
		}
	}
}

void CPacketIOManager::InitThreadPool(int ThreadCnt)
{
	m_ThreadCnt = ThreadCnt;
	m_vecThreadWorker.reserve(m_ThreadCnt);

	for (int i = 0; i < m_ThreadCnt; ++i)
	{
		std::shared_ptr<CTaskWorker> Worker = std::make_shared<CTaskWorker>();
		m_vecThreadWorker.push_back(Worker);	
	}

}

const int CPacketIOManager::GetLeastWorkThreadIdx()
{
	int Idx = 0;
	size_t MinCnt = m_vecThreadWorker[0]->GetTaskCount();

	for (int i = 1; i < m_ThreadCnt; ++i)
	{
		size_t CurcCnt = m_vecThreadWorker[i]->GetTaskCount();
		if (MinCnt > CurcCnt)
		{
			Idx = i;
			MinCnt = CurcCnt;
		}
	}

	return Idx;
}

void CPacketIOManager::EchoChattingData(char* EchoData, int DataLength)
{
	// 이미 수신된 상태이다.
	CSessionManager* SMInst = CSessionManager::GetInst();
	std::unordered_map<CRemoteClient*, std::shared_ptr<CRemoteClient>> mapRemoteClient = SMInst->GetAllSessions();

	auto iter = mapRemoteClient.begin();
	auto iterEnd = mapRemoteClient.end();
	for (; iter != iterEnd; ++iter)
	{
		//Overlaaped 송신
		if (iter->second->OverlappedSend(EchoData, DataLength) == SOCKET_ERROR)
		{
			std::cerr << "Error in WSASend" << std::endl;
		}
	}
}
