
#pragma once
#include "BaseTask.h"
#include "ServerShared.h"

class CIocpEventTask :
    public CBaseTask
{
public:
	CIocpEventTask();
	CIocpEventTask(OVERLAPPED_ENTRY IocpEvent);
	virtual ~CIocpEventTask();

protected:
	OVERLAPPED_ENTRY m_IocpEvent;

public:
	virtual void RunTask();
};

