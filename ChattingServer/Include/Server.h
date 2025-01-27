#pragma once

#include "ServerShared.h"

class CRemoteClient;

class CServer
{
public:
	CServer();
	CServer(int ThreadCount);
	~CServer();
	
public:
	bool InitServer();
	void CloseServer();

public:
	int Run();
};

