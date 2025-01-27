
#include "Server.h"
#include "RemoteClient.h"
#include "PacketManager.h"

// TODO : TCP 스트림 일부만 송신하고 리턴하는 경우도 고려, 송수신 기능 패킷 매니저로 이관, 클라이언트 관리 기능 세션 매니저로 이관

CServer::CServer() :	m_isStopWorking(false),
						m_ListenSocket(SocketType::Tcp)
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

	CPacketManager::DestroySingleInst();
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

	std::cout << "Chatting Server Starting...\n";

	return true;
}

void CServer::IocpLoop()
{
	while (!m_isStopWorking)
	{
		IocpEvents ReadEvents;
		m_Iocp.Wait(ReadEvents, 100);

		int EventCnt = ReadEvents.m_eventCount;
		for (int i = 0; i < EventCnt; ++i)
		{
			auto& Event = ReadEvents.m_events[i];
			if (Event.lpCompletionKey == 0)//리슨소켓일 경우
			{
				m_ListenSocket.m_isReadOverlapped = false;
				//Accept는 이미 Init에서 해주었다.
				if (m_RemoteClientCandidate->m_TcpSocket.UpdateAcceptContext(m_ListenSocket) != 0)
				{
					//에러가 나면 리슨소켓 닫기
					m_ListenSocket.Close();
				}

				else
				{
					std::shared_ptr<CRemoteClient> RemoteClient = m_RemoteClientCandidate;

					// 새 TCP 소켓 IOCP에 추가
					m_Iocp.Add(m_RemoteClientCandidate->m_TcpSocket, RemoteClient.get());

					// I/O 수신 요청을 걸어둔다.
					if (RemoteClient->m_TcpSocket.ReceiveOverlapped() != 0
						&& WSAGetLastError() != ERROR_IO_PENDING)
					{
						// 에러. 소켓 정리
						RemoteClient->m_TcpSocket.Close();
					}
					else
					{
						//클라이언트 서버 입장 처리
						ProcessClientEnter(RemoteClient);
					}

					// 계속해서 새 연결 받기 위해 리슨소켓도 overlapped I/O 걸기.
					m_RemoteClientCandidate = std::make_shared<CRemoteClient>(SocketType::Tcp);
					std::string errorText;
					if (!m_ListenSocket.AcceptOverlapped(m_RemoteClientCandidate->m_TcpSocket, errorText)
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
				std::shared_ptr<CRemoteClient> RemoteClient = m_mapRemoteClient[(CRemoteClient*)Event.lpCompletionKey];

				if (Event.dwNumberOfBytesTransferred <= 0)
				{
					int a = 0;
				}

				if (RemoteClient)
				{
					// 이미 수신된 상태이다.
					RemoteClient->m_TcpSocket.m_isReadOverlapped = false;
					int DataLength = Event.dwNumberOfBytesTransferred;

					if (DataLength <= 0)
					{
						// 읽은 결과가 0 혹은 음수이므로 끝내자
						ProcessClientLeave(RemoteClient);
					}
					else
					{
						// 이미 수신된 상태이다.
						char* EchoData = RemoteClient->m_TcpSocket.m_receiveBuffer;
						
						auto iter = m_mapRemoteClient.begin();
						auto iterEnd = m_mapRemoteClient.end();
						for(;iter!=iterEnd;++iter)
						{
							//Overlaaped 송신
							if (iter->second->m_TcpSocket.OverlappedSend(EchoData, DataLength) == SOCKET_ERROR)
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
							// I/O를 걸었다. 완료를 대기 상태로 바꾸자.
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
	// 백그라운드 진행 중인 Overlapped I/O 완료를 모두 체크 후 종료
	m_ListenSocket.Close();
	for (auto i : m_mapRemoteClient)
	{
		i.second->m_TcpSocket.Close();
	}

	std::cout << "Chatting Server Closing...\n";
	while (m_mapRemoteClient.size() > 0 || m_ListenSocket.m_isReadOverlapped)
	{
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

		// I/O completion 발생하더라도 정리 한다.
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

	std::cout << "Chatting Server OFF.\n";
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

