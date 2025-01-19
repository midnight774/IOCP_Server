
#include "Server.h"

#ifdef _DEBUG

#pragma comment(lib, "SockComm_Debug.lib")

#else

#pragma comment(lib, "SockComm.lib")

#endif // _DEBUG


int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(165);

	CServer* MainChatServer = new CServer;
	int Ret = -1;

	if (MainChatServer->InitServer())
	{
		Ret = MainChatServer->Run();
	}

	delete MainChatServer;

	return Ret;
}