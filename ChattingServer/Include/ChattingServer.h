#pragma once

#include "ServerShared.h"

class CRemoteClient;

class CChattingServer
{
public:
	CChattingServer();
	CChattingServer(int ThreadCount);
	~CChattingServer();
	
public:
	bool InitServer();
	void CloseServer();

public:
	int Run();
};

