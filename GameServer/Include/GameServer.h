
#pragma once

#include "ServerShared.h"

class CGameServer
{
public:
	CGameServer();
	CGameServer(int ThreadCount);
	~CGameServer();

public:
	bool InitServer();
	void CloseServer();

public:
	int Run();
};

