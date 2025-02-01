
#include "GameServer.h"

#ifdef _DEBUG

#pragma comment(lib, "SockComm_Debug.lib")

#else

#pragma comment(lib, "SockComm.lib")

#endif // _DEBUG

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(169);

	CGameServer* MainGameServer = new CGameServer(4);
	int Ret = -1;

	if (MainGameServer->InitServer())
	{
		Ret = MainGameServer->Run();
	}

	SAFE_DELETE(MainGameServer);

	return Ret;
}
