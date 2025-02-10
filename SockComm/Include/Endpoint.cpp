#include "stdafx.h"
#include "Endpoint.h"


CEndpoint::CEndpoint()
{
	memset(&m_IPv4Endpoint, 0, sizeof(m_IPv4Endpoint));
	m_IPv4Endpoint.sin_family = AF_INET;

}

CEndpoint::CEndpoint(const char* Address, int PortNum)
{
	memset(&m_IPv4Endpoint, 0, sizeof(m_IPv4Endpoint));
	m_IPv4Endpoint.sin_family = AF_INET;
	inet_pton(AF_INET, Address, &m_IPv4Endpoint.sin_addr);
	m_IPv4Endpoint.sin_port = htons((uint16_t)PortNum);
}

CEndpoint::CEndpoint(sockaddr_in& Addr)
{
	memset(&m_IPv4Endpoint, 0, sizeof(sockaddr_in));
	memcpy(&m_IPv4Endpoint, &Addr, sizeof(sockaddr_in));
}

CEndpoint::~CEndpoint()
{
}

CEndpoint CEndpoint::Any;

std::string CEndpoint::ToString()
{
	//IP 주소 추가
	char Address[1000];
	Address[0] = 0;
	inet_ntop(AF_INET, &m_IPv4Endpoint.sin_addr, Address, sizeof(Address) - 1);

	//포트 넘버까지 추가
	char ResultString[1000];
	sprintf(ResultString, "%s:%d", ResultString, htons(m_IPv4Endpoint.sin_port));

	return ResultString;
}
