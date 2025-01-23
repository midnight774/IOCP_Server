#pragma once

#include "ServerShared.h"

class CPacketManager
{
public:
	bool Init();

private:

	DECLARE_SINGLETON(CPacketManager);
};

