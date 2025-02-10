
#include "GameServer.h"

#ifdef _DEBUG

#pragma comment(lib, "SockComm_Debug.lib")

#else

#pragma comment(lib, "SockComm.lib")

#endif // _DEBUG

CGameServer* MainGameServer = nullptr;

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		SAFE_DELETE(MainGameServer);
		return TRUE;
	default:
		return FALSE;
	}
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(169);

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	MainGameServer = new CGameServer(4);
	int Ret = -1;

	if (MainGameServer->InitServer())
	{
		Ret = MainGameServer->Run();
	}

	SAFE_DELETE(MainGameServer);

	return Ret;
}
