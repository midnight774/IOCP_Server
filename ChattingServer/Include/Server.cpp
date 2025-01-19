
#include "Server.h"
#include "RemoteClient.h"
#include "PacketManager.h"

// TODO : TCP 스트림 일부만 송신하고 리턴하는 경우도 고려, 송수신 기능 패킷매니저로 이관

CServer::CServer() :	m_isStopWorking(false),
						m_ListenSocket(SocketType::Tcp),
						m_Iocp(1)
{
	m_ListenSocket.Bind(Endpoint("0.0.0.0", 5555));
}

CServer::CServer(int ThreadCount) : m_isStopWorking(false),
									m_ListenSocket(SocketType::Tcp),
									m_Iocp(ThreadCount)
{
	m_ListenSocket.Bind(Endpoint("0.0.0.0", 5555));
}

CServer::~CServer()
{
	auto iter = m_mapRemoteClient.begin();
	auto iterEnd = m_mapRemoteClient.end();

	for (; iter != iterEnd;)
	{
		iter = m_mapRemoteClient.erase(iter);
	}

	CPacketManager::DestroyInst();
}

bool CServer::InitServer()
{
	m_ListenSocket.Listen();
	m_Iocp.Add(m_ListenSocket, nullptr);

	//Overlapped Accept
	m_RemoteClientCandidate = std::make_shared<CRemoteClient>(SocketType::Tcp);

	std::string ErrorText;
	if (!m_ListenSocket.AcceptOverlapped(m_RemoteClientCandidate->m_TcpSocket, ErrorText)
		&& WSAGetLastError() != ERROR_IO_PENDING)
	{
		throw Exception("Overlapped AcceptEx Failed");
	}

	m_ListenSocket.m_isReadOverlapped = true;

	if (!CPacketManager::GetInst()->Init())
	{
		return false;
	}

	std::cout << "서버 시작\n";
	std::cout << "CLT-C 누를시 종료\n";

	return true;
}

void CServer::IocpLoop()
{
	while (!m_isStopWorking)
	{
		//100ms까지 IOCP 이벤트 대기 
		IocpEvents ReadEvents;
		m_Iocp.Wait(ReadEvents, 100);

		int EventCnt = ReadEvents.m_eventCount;
		for (int i = 0; i < EventCnt; ++i)
		{
			auto& Event = ReadEvents.m_events[i];
			if (Event.lpCompletionKey == 0)//리슨소켓일 경우
			{
				m_ListenSocket.m_isReadOverlapped = false;
				//Accept는 위에서 완료된 상태이다.
				if (m_RemoteClientCandidate->m_TcpSocket.UpdateAcceptContext(m_ListenSocket) != 0)
				{
					//리슨소켓을 닫았던지 하면 여기서 에러날거다. 그러면 리슨소켓 불능상태로 만들자.
					m_ListenSocket.Close();
				}

				else
				{
					std::shared_ptr<CRemoteClient> RemoteClient = m_RemoteClientCandidate;

					// 새 TCP 소켓 IOCP에 추가한다.
					m_Iocp.Add(m_RemoteClientCandidate->m_TcpSocket, RemoteClient.get());

					// overlapped 수신을 받을 수 있어야 하므로 여기서 I/O 수신 요청을 걸어둔다.
					if (RemoteClient->m_TcpSocket.ReceiveOverlapped() != 0
						&& WSAGetLastError() != ERROR_IO_PENDING)
					{
						// 에러. 소켓을 정리하자. 그리고 그냥 버리자.
						RemoteClient->m_TcpSocket.Close();
					}
					else
					{
						//클라이언트 서버 입장 처리
						ProcessClientEnter(RemoteClient);
					}

					// 계속해서 소켓 받기를 해야 하므로 리슨소켓도 overlapped I/O를 걸자.
					m_RemoteClientCandidate = std::make_shared<CRemoteClient>(SocketType::Tcp);
					std::string errorText;
					if (!m_ListenSocket.AcceptOverlapped(m_RemoteClientCandidate->m_TcpSocket, errorText)
						&& WSAGetLastError() != ERROR_IO_PENDING)
					{
						// 에러나면 리슨소켓 불능 상태로 남기자. 
						m_ListenSocket.Close();
					}
					else
					{
						// 리슨소켓은 연결이 들어옴을 기다리는 상태가 되었다.
						m_ListenSocket.m_isReadOverlapped = true;
					}
				}
			}
			else  // TCP 연결 소켓이면
			{
				// 받은 데이터를 그대로 회신한다.
				std::shared_ptr<CRemoteClient> RemoteClient = m_mapRemoteClient[(CRemoteClient*)Event.lpCompletionKey];

				if (Event.dwNumberOfBytesTransferred <= 0)
				{
					int a = 0;
				}

				if (RemoteClient)
				{
					// 이미 수신된 상태이다.
					RemoteClient->m_TcpSocket.m_isReadOverlapped = false;
					int ec = Event.dwNumberOfBytesTransferred;

					if (ec <= 0)
					{
						// 읽은 결과가 0 혹은 음수이므로 끝내자
						ProcessClientLeave(RemoteClient);
					}
					else
					{
						// 이미 수신된 상태이다.
						char* echoData = RemoteClient->m_TcpSocket.m_receiveBuffer;
						int echoDataLength = ec;

						// 원칙대로라면 TCP 스트림 특성상 일부만 송신하고 리턴하는 경우도 고려해야 한다.
						// 이후에 Overlapped 송신으로변경해야한다.						
						//*********************************************************
						
						auto iter = m_mapRemoteClient.begin();
						auto iterEnd = m_mapRemoteClient.end();
						for(;iter!=iterEnd;++iter)
						{
							if (iter->second == RemoteClient)
								continue;

							//iter->second->m_TcpSocket.Send(echoData, ec);

							//Overlaaped 송신
							if (iter->second->m_TcpSocket.OverlappedSend(echoData, ec) == SOCKET_ERROR)
							{
								std::cerr << "Error in WSASend" << std::endl;
							}
						}

						// 다시 수신을 받으려면 overlapped I/O를 걸어야 한다.
						if (RemoteClient->m_TcpSocket.ReceiveOverlapped() != 0
							&& WSAGetLastError() != ERROR_IO_PENDING)
						{
							ProcessClientLeave(RemoteClient);
						}
						else
						{
							// I/O를 걸었다. 완료를 대기하는 중 상태로 바꾸자.
							RemoteClient->m_TcpSocket.m_isReadOverlapped = true;
						}
					}
				}
			}
		}
	}
}

void CServer::ProcessClientEnter(std::shared_ptr<CRemoteClient> RemoteClient)
{
	// I/O를 걸었다. 완료를 대기하는 중 상태로 바꾸자.
	RemoteClient->m_TcpSocket.m_isReadOverlapped = true;

	// 새 클라이언트 목록에 추가.
	m_mapRemoteClient.insert({ RemoteClient.get(), RemoteClient });

	std::cout << "Client joined. There are " << m_mapRemoteClient.size() << " connections.\n";
}

void CServer::ProcessClientLeave(std::shared_ptr<CRemoteClient> RemoteClient)
{
	RemoteClient->m_TcpSocket.Close();
	m_mapRemoteClient.erase(RemoteClient.get());

	std::cout << "Client left. There are " << m_mapRemoteClient.size() << " connections.\n";
}

void CServer::CloseServer()
{
	// 루프가 종료됐다. 서버를 종료 하자
	// 그러나 overlapped I/O가 모두 완료되기 전에는 그냥 무작정 나가면 안된다. 
	// 백그라운드 진행 중인 Overlapped I/O 완료를 모두 체크하고 종료 하자.
	m_ListenSocket.Close();
	for (auto i : m_mapRemoteClient)
	{
		i.second->m_TcpSocket.Close();
	}

	std::cout << "서버를 종료중\n";
	while (m_mapRemoteClient.size() > 0 || m_ListenSocket.m_isReadOverlapped)
	{
		// I/O completion이 없는 상태의 RemoteClient를 제거한다.
		auto iter = m_mapRemoteClient.begin();
		auto iterEnd = m_mapRemoteClient.end();

		for (; iter != iterEnd;)
		{
			if (!iter->second->m_TcpSocket.m_isReadOverlapped)
			{
				iter = m_mapRemoteClient.erase(iter);
			}
			else
				iter++;
		}

		// I/O completion이 발생하면 더 이상 Overlapped I/O를 걸지 말고 '이제 정리해도 돼...'를 플래깅한다.
		IocpEvents readEvents;
		m_Iocp.Wait(readEvents, 100);

		// 받은 이벤트 각각을 처리합니다.
		for (int i = 0; i < readEvents.m_eventCount; i++)
		{
			auto& readEvent = readEvents.m_events[i];
			if (readEvent.lpCompletionKey == 0) // 리슨소켓이면
			{
				m_ListenSocket.m_isReadOverlapped = false;
			}
			else
			{
				std::shared_ptr<CRemoteClient> RemoteClient = m_mapRemoteClient[(CRemoteClient*)readEvent.lpCompletionKey];
				if (RemoteClient)
				{
					RemoteClient->m_TcpSocket.m_isReadOverlapped = false;
				}
			}
		}
	}

	std::cout << "서버 종료.\n";
}

int CServer::Run()
{
	try
	{
		IocpLoop();	
		CloseServer();
	}
	catch (Exception& e)
	{
		std::cout << "Exception! " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

