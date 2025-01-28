
#pragma once
#include "SockComm.h"

class CBaseTask
{
public:
	CBaseTask();
	virtual ~CBaseTask() = 0;

public:
	virtual void RunTask() = 0;

};

