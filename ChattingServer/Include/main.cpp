
#include "ChattingServer.h"

#ifdef _DEBUG

#pragma comment(lib, "SockComm_Debug.lib")

#else

#pragma comment(lib, "SockComm.lib")

#endif // _DEBUG

CChattingServer* MainChatServer = nullptr;

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
		SAFE_DELETE(MainChatServer);
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

	MainChatServer = new CChattingServer(4);
	int Ret = -1;

	if (MainChatServer->InitServer())
	{
		Ret = MainChatServer->Run();
	}

	delete MainChatServer;

	return Ret;
}