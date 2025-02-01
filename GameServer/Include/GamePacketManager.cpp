
#include "GamePacketManager.h"
#include "ClientSession.h"
#include "GameSessionManager.h"
#include "TaskWorker.h"

DEFINITION_SINGLETON(CGamePacketManager);

//TODO : 스레드 Task 제작, IOCP에 UDP소켓 추가할 때 어떤 주소로 지정할지 고민

CGamePacketManager::CGamePacketManager() :
	m_ListenSocket(SocketType::Tcp),
	m_UdpSocket(SocketType::Udp),
	m_IsStopWorking(false)
{
	m_ListenSocket.Bind(CEndpoint("0.0.0.0", 5555));
	m_UdpSocket.Bind(CEndpoint("0.0.0.0", 5555));
}

CGamePacketManager::~CGamePacketManager()
{
	m_ListenSocket.Close();

	// 백그라운드 진행 중인 Overlapped I/O 완료를 모두 체크 후 종료
	while (m_ListenSocket.m_isReadOverlapped)
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
				std::shared_ptr<CClientSession> ClientSession = CGameSessionManager::GetInst()->FindClient((CClientSession*)readEvent.lpCompletionKey);
				if (ClientSession)
				{
					ClientSession->m_TcpSocket.m_isReadOverlapped = false;
				}
			}
		}
	}
}

bool CGamePacketManager::Init()
{
	m_ListenSocket.Listen();
	m_Iocp.Add(m_ListenSocket, nullptr); // Completion Key == nullptr == 0
	m_Iocp.Add(m_UdpSocket, &m_UdpSocket);

	//Overlapped Accept
	m_PendingClient = std::make_shared<CClientSession>(SocketType::Tcp);

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

void CGamePacketManager::IocpLoop()
{
	IocpEvents ReadEvents;
	m_Iocp.Wait(ReadEvents, 100);

	int EventCnt = ReadEvents.m_EventCount;
	for (int i = 0; i < EventCnt; ++i)
	{
		ProcessIocpEvent(ReadEvents.m_ArrEvents[i]);
		/*std::shared_ptr<CIocpEventTask> Task = std::make_shared<CIocpEventTask>(ReadEvents.m_ArrEvents[i]);
		int Idx = GetLeastWorkThreadIdx();
		m_vecThreadWorker[Idx]->PushTask(Task);*/
	}
}

void CGamePacketManager::ProcessIocpEvent(const OVERLAPPED_ENTRY& Event)
{
	std::lock_guard<std::mutex> Lock(m_IOMutex);

	CGameSessionManager* SMInst = CGameSessionManager::GetInst();

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
			std::shared_ptr<CClientSession> ClientSession = m_PendingClient;

			// 새 TCP 소켓 IOCP에 추가
			m_Iocp.Add(m_PendingClient->m_TcpSocket, ClientSession.get());

			// I/O 수신 요청을 걸어둔다.
			if (ClientSession->m_TcpSocket.ReceiveOverlapped() != 0
				&& WSAGetLastError() != ERROR_IO_PENDING)
			{
				// 에러
				ClientSession->m_TcpSocket.Close();
			}
			else
			{
				//클라이언트 서버 입장 처리
				SMInst->InsertClient(ClientSession);
			}

			// 계속해서 새 연결 받기 위해 리슨소켓 overlapped I/O 걸기.
			m_PendingClient.reset();
			m_PendingClient = std::make_shared<CClientSession>(SocketType::Tcp);
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
	else if ((CSocket*)Event.lpCompletionKey == &m_UdpSocket) //UDP 통신인 경우
	{
		if (Event.lpOverlapped == &m_UdpSocket.m_SendOverlappedStruct) // 송신 이벤트인 경우
		{
		}

		else if (Event.lpOverlapped == &m_UdpSocket.m_ReceiveOverlappedStruct)// 수신 이벤트인 경우
		{
			char* Data = m_UdpSocket.m_ReceiveBuffer;
			
			Packet_Type Type = (Packet_Type)-1;
			memcpy(&Type, Data, sizeof(Packet_Type));
			
			switch (Type)
			{
			case Packet_Type::CharacterMove:
			{
				//유효한 데이터 크기인지 체크 해야한다.
				CharacterMoveData MoveData;
				memcpy(&MoveData, Data + sizeof(Packet_Type), sizeof(CharacterMoveData));

				//이후 처리
			}
				break;


			}
			

		}
	}

	else  // TCP 연결 소켓이면
	{
		std::shared_ptr<CClientSession> ClientSession = SMInst->FindClient((CClientSession*)Event.lpCompletionKey);

		if (ClientSession)
		{
			if (Event.lpOverlapped == &(ClientSession->m_TcpSocket.m_SendOverlappedStruct)) // 송신 이벤트인 경우
			{
			}

			else if (Event.lpOverlapped == &(ClientSession->m_TcpSocket.m_ReceiveOverlappedStruct))// 수신 이벤트인 경우
			{
				ClientSession->SetOverlappedReceiveFlag(false);
				int DataLength = Event.dwNumberOfBytesTransferred;

				if (DataLength <= 0)
				{
					SMInst->EraseClient(ClientSession);
				}
				else
				{
					char* EchoData = ClientSession->m_TcpSocket.m_ReceiveBuffer;
					//EchoGameData(EchoData, DataLength);

					if ((Packet_Type)EchoData[0] == Packet_Type::Login)
					{
						LoginData Data;
						memset(&Data, 0, sizeof(Data));
						
						int Size = sizeof(Packet_Type);
						memcpy(&Data.IdLength, EchoData + Size, sizeof(int));

						Size += sizeof(int);
						memcpy(&Data.Id, EchoData + Size, Data.IdLength);

						Size += Data.IdLength;
						memcpy(&Data.PwLength, EchoData + Size, sizeof(int));
						
						Size += sizeof(int);
						memcpy(&Data.Password, EchoData + Size, Data.PwLength);

						std::cout << Data.Id << '\t' << Data.Password << '\n';
					}



					// 다시 수신을 받으려면 overlapped I/O를 걸어야 한다.
					if (ClientSession->m_TcpSocket.ReceiveOverlapped() != 0
						&& WSAGetLastError() != ERROR_IO_PENDING)
					{
						//오류시 삭제
						SMInst->EraseClient(ClientSession);
					}
					else
					{
						// I/O를 걸었다. 완료를 대기 상태로 바꾸자.
						ClientSession->SetOverlappedReceiveFlag(true);
					}
				}
			}

		}
	}
}

void CGamePacketManager::InitThreadPool(int ThreadCnt)
{
	m_ThreadCnt = ThreadCnt;
	m_vecThreadWorker.reserve(m_ThreadCnt);

	for (int i = 0; i < m_ThreadCnt; ++i)
	{
		std::shared_ptr<CTaskWorker> Worker = std::make_shared<CTaskWorker>();
		m_vecThreadWorker.push_back(Worker);
	}

}

const int CGamePacketManager::GetLeastWorkThreadIdx()
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

void CGamePacketManager::EchoGameData(char* EchoData, int DataLength)
{
	// 이미 수신된 상태이다.
	CGameSessionManager* SMInst = CGameSessionManager::GetInst();
	std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>> mapClientSession = SMInst->GetAllSessions();

	auto iter = mapClientSession.begin();
	auto iterEnd = mapClientSession.end();
	for (; iter != iterEnd; ++iter)
	{
		//Overlaaped 송신
		if (iter->second->OverlappedSend(EchoData, DataLength) == SOCKET_ERROR)
		{
			std::cerr << "Error in WSASend" << std::endl;
		}
	}
}
