
#include "GamePacketManager.h"
#include "ClientSession.h"
#include "GameSessionManager.h"
#include "TaskWorker.h"
#include "GameLogicManager.h"
#include "GameObjectInfo.h"
#include "PacketSerializer.h"

DEFINITION_SINGLETON(CGamePacketManager);

//TODO : 스레드 Task 제작, IOCP에 UDP소켓 추가할 때 어떤 주소로 지정할지 고민

CGamePacketManager::CGamePacketManager() :
	m_ListenSocket(SocketType::Tcp),
	m_UdpSocket(SocketType::Udp),
	m_IsStopWorking(false)
{
	m_ListenSocket.Bind(CEndpoint("0.0.0.0", 5555));
	m_UdpSocket.Bind(CEndpoint("0.0.0.0", 50500));
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

	m_UdpSocket.ReceiveFromOverlapped();

	m_ListenSocket.m_isReadOverlapped = true;

	m_ThreadCnt = 12;

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
				EchoClientEnterData(ClientSession);
				SMInst->InsertClient(ClientSession);
			}

			// 계속해서 새 연결 받기 위해 리슨소켓 overlapped I/O 걸기.
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
			int DataLength = Event.dwNumberOfBytesTransferred;
			char* Data = m_UdpSocket.m_ReceiveBuffer;
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
					EchoClientLeaveData(ClientSession);
				}
				else
				{
					char* RecvData = ClientSession->m_TcpSocket.m_ReceiveBuffer;
					//EchoGameData(EchoData, DataLength);

					int DataLength = Event.dwNumberOfBytesTransferred;

					Packet_Type Type = (Packet_Type)-1;
					memcpy(&Type, RecvData, sizeof(Packet_Type));

					switch (Type)
					{
					case Packet_Type::Login:
					{
						//유효한 데이터 크기인지 체크 해야한다.
						LoginData Data;
						memset(&Data, 0, sizeof(Data));

						int Size = sizeof(Packet_Type);
						memcpy(&Data.IdLength, RecvData + Size, sizeof(int));
						Size += sizeof(int);
						memcpy(&Data.Id, RecvData + Size, Data.IdLength);
						Size += Data.IdLength;
						memcpy(&Data.PwLength, RecvData + Size, sizeof(int));
						Size += sizeof(int);
						memcpy(&Data.Password, RecvData + Size, Data.PwLength);

						std::cout << Data.Id << '\t' << Data.Password << '\n';
						//이후 처리
						break;
					}
					case Packet_Type::Logout:
					{

						break;
					}
					case Packet_Type::Register:
					{

						break;
					}
					case Packet_Type::CharacterMove:
					{
						ProcessCharacterMovePacket(RecvData, DataLength);
						break;
					}
					case Packet_Type::Endpoint:
					{
						sockaddr_in Addr;
						int Size = sizeof(Packet_Type);
						memcpy(&Addr, RecvData + Size, sizeof(sockaddr_in));

						ClientSession->m_ClientUdpEndpoint.m_IPv4Endpoint = Addr;
					}


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
	/*m_ThreadCnt = ThreadCnt;
	m_vecIOCPWorker.reserve(m_ThreadCnt);

	for (int i = 0; i < m_ThreadCnt; ++i)
	{
		std::shared_ptr<CTaskWorker> Worker = std::make_shared<CTaskWorker>();
		m_vecIOCPWorker.push_back(Worker);
	}*/
}

const int CGamePacketManager::GetLeastWorkThreadIdx()
{
	int Idx = 0;
	size_t MinCnt = m_vecIOCPWorker[0]->GetTaskCount();

	for (int i = 1; i < m_ThreadCnt; ++i)
	{
		size_t CurcCnt = m_vecIOCPWorker[i]->GetTaskCount();
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
		CEndpoint Addr = iter->second->m_ClientUdpEndpoint;

		//Test용
		inet_pton(AF_INET, "127.0.0.1", &Addr.m_IPv4Endpoint.sin_addr);
		
		if (m_UdpSocket.OverlappedSendTo(EchoData, DataLength, Addr) == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();
			if(Error == ERROR_IO_PENDING)
				std::cerr << "Error in WSASend" << std::endl;
		}		
	}
}

void CGamePacketManager::EchoClientEnterData(std::shared_ptr<CClientSession> Session)
{
	CGameSessionManager* SMInst = CGameSessionManager::GetInst();
	std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>> mapClientSession = SMInst->GetAllSessions();

	auto iter = mapClientSession.begin();
	auto iterEnd = mapClientSession.end();

	char EchoData[1024] = {};
	int Size = 0;
	Packet_Type Type = Packet_Type::Spawn;

	std::vector<SpawnCharacterData> vecSpawnData;
	vecSpawnData.reserve(mapClientSession.size() + 1);
	vecSpawnData.emplace_back(SMInst->GetNewClientID(), 0, false, 500.f, 500.f, 0);//초기 생성시 테스트용 좌표로 생성

	//기존 유저들에게 보낼 정보 생성
	CPacketSerializer::SerializeCharacterSpawns(vecSpawnData, EchoData, Size);
	
	for (; iter != iterEnd; ++iter)
	{
		//Overlaaped 송신
		if (iter->second->m_TcpSocket.OverlappedSend(EchoData, Size) == SOCKET_ERROR)
		{
			std::cerr << "Error in WSASend" << std::endl;
		}

		std::shared_ptr<CGameObjectInfo> pInfo = iter->second->m_ClientCharacterInfo;
		vecSpawnData.emplace_back(pInfo->GetObjectID(), 0, false, pInfo->GetPos().x, pInfo->GetPos().y, pInfo->GetLastObjectView());
	}

	//새로 들어온 유저에게 기존 유저 정보 보내준다.
	vecSpawnData[0].IsLocal = true;
	int TotalSize = 0;
	memset(EchoData, 0, 1024);
	CPacketSerializer::SerializeCharacterSpawns(vecSpawnData, EchoData, TotalSize);

	if (Session->m_TcpSocket.OverlappedSend(EchoData, TotalSize) == (-1))
	{
		std::cerr << "Error in WSASend" << std::endl;
	}
}

void CGamePacketManager::EchoClientLeaveData(std::shared_ptr<CClientSession> Session)
{
	CGameSessionManager* SMInst = CGameSessionManager::GetInst();
	std::unordered_map<CClientSession*, std::shared_ptr<CClientSession>> mapClientSession = SMInst->GetAllSessions();

	auto iter = mapClientSession.begin();
	auto iterEnd = mapClientSession.end();

	char EchoData[512] = {};
	int Size = 0;
	Packet_Type Type = Packet_Type::Despawn;

	memcpy(EchoData, &Type, sizeof(UINT8));
	Size += sizeof(UINT8);

	int ObjID = Session->m_ClientCharacterInfo->GetObjectID();
	memcpy(EchoData + Size, &ObjID, sizeof(int));
	Size += sizeof(int);

	int TotalSize = Size;

	for (; iter != iterEnd; ++iter)
	{
		//Overlaaped 송신
		if (iter->second->m_TcpSocket.OverlappedSend(EchoData, Size) == SOCKET_ERROR)
		{
			std::cerr << "Error in WSASend" << std::endl;
		}
	}
}

void CGamePacketManager::PrcoessLoginPacket(char* Data, int DataLengt)
{
}

void CGamePacketManager::ProcessCharacterMovePacket(char* Data, int DataLength)
{
	CharacterMoveData MoveData;

	memset(&MoveData, 0, sizeof(Data));
	int Size = sizeof(Packet_Type);
	LARGE_INTEGER Time = {};
	memcpy(&Time, Data + Size, sizeof(LARGE_INTEGER));
	Size += sizeof(LARGE_INTEGER);
	memcpy(&MoveData.ObjectID, Data + Size, sizeof(UINT));
	Size += sizeof(UINT);
	memcpy(&MoveData.Dir, Data + Size, sizeof(UINT8));
	Size += sizeof(UINT8);
	memcpy(&MoveData.PosX, Data + Size, sizeof(float));
	Size += sizeof(float);
	memcpy(&MoveData.PosY, Data + Size, sizeof(float));
	Size += sizeof(float);
	memcpy(&MoveData.IsEnd, Data + Size, sizeof(bool));

	std::shared_ptr<CClientSession> Session = CGameSessionManager::GetInst()->FindSessionByObjectID(MoveData.ObjectID);

	Vector3 Dir;
	switch (MoveData.Dir)
	{
	case 0:
		Dir = Vector3(-1.f, 0.f, 0.f);
		break;
	case 1:
		Dir = Vector3(0.f, 1.f, 0.f);
		break;
	case 2:
		Dir = Vector3(0.f, -1.f, 0.f);
		break;
	case 3:
		Dir = Vector3(1.f, 0.f, 0.f);
		break;
	}

	if (MoveData.IsEnd)
	{
		Session->m_ClientCharacterInfo->AddMoveDir(Dir * -1);
	}

	else
	{
		Session->m_ClientCharacterInfo->AddMoveDir(Dir);
		Session->m_ClientCharacterInfo->SetPos(Vector3(MoveData.PosX, MoveData.PosY, 0.f));
	}

	Session->m_ClientCharacterInfo->SetLastUpdateTime(Time);

	CGameLogicManager::GetInst()->AddUpdateClient(Session);

}
