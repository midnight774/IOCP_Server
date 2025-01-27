#pragma once

#include <Ws2tcpip.h>

class CEndpoint
{
public:
	CEndpoint();
	CEndpoint(const char* Address, int PortNum);
	~CEndpoint();

	sockaddr_in m_IPv4Endpoint;
	static CEndpoint Any;
	std::string ToString();
};

