#include "PacketIOManager.h"
#include "RemoteClient.h"
#include "SessionManager.h"

DEFINITION_SINGLETON(CPacketIOManager);

CPacketIOManager::CPacketIOManager()	:
	m_ListenSocket(SocketType::Tcp),
	m_isStopWorking(false)
{
	m_ListenSocket.Bind(Endpoint("0.0.0.0", 5555));

}

CPacketIOManager::~CPacketIOManager()
{
	m_ListenSocket.Close();

	// 백그라운드 진행 중인 Overlapped I/O 완료를 모두 체크 후 종료
	while(m_ListenSocket.m_isReadOverlapped)
	{
		IocpEvents readEvents;
		m_Iocp.Wait(readEvents, 100);

		// 받은 이벤트 각각을 처리
		for (int i = 0; i < readEvents.m_eventCount; i++)
		{
			auto& readEvent = readEvents.m_events[i];
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
	while (!m_isStopWorking)
	{
		IocpEvents ReadEvents;
		m_Iocp.Wait(ReadEvents, 100);

		int EventCnt = ReadEvents.m_eventCount;
		for (int i = 0; i < EventCnt; ++i)
		{
			ProcessIocpEvent(ReadEvents.m_events[i]);
		}
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

		if (Event.dwNumberOfBytesTransferred <= 0)
		{
			int a = 0;
		}

		if (RemoteClient)
		{
			// 이미 수신된 상태이다.
			RemoteClient->SetOverlappedReadFlag(false);
			int DataLength = Event.dwNumberOfBytesTransferred;

			if (DataLength <= 0)
			{
				// 읽은 결과가 0 혹은 음수이므로 끝내자
				SMInst->EraseClient(RemoteClient);
			}
			else
			{
				// 이미 수신된 상태이다.
				char* EchoData = RemoteClient->m_TcpSocket.m_receiveBuffer;
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
					RemoteClient->SetOverlappedReadFlag(true);
				}
			}
		}
	}
}

void CPacketIOManager::InitThreadPool(int ThreadCnt)
{
}

const int CPacketIOManager::GetLeastWorkThreadIdx()
{
	return 0;
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
