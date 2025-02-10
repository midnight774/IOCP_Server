#pragma once
#include "BaseTask.h"

class CUpdateCharacterTask :
    public CBaseTask
{
public:
	CUpdateCharacterTask();
	virtual ~CUpdateCharacterTask();

protected:

public:
	virtual void RunTask();
};

