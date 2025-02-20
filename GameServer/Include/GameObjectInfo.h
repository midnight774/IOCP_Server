#pragma once
#include "ServerShared.h"

class CGameObjectInfo
{
public:
	CGameObjectInfo();
	~CGameObjectInfo();

private:
	std::mutex m_Mtx;
	UINT	m_ObjectID;
	Vector3 m_Pos;
	Vector3	m_MoveDir;
	UINT8	m_ObjectView;
	bool	m_IsMoving;
	float	m_MoveSpeed;
	LARGE_INTEGER	m_LastUpdateTime;
	//나중에 공격력 등의 정보 추가

public:
	void SetObjectID(UINT ID)
	{
		m_ObjectID = ID;
	}

	void SetMoveSpeed(float Speed) 
	{
		m_MoveSpeed = Speed;
	}
	void SetPos(const Vector3& Pos)
	{
		m_Pos = Pos;
	}
	void AddMoveDir(const Vector3& Dir)
	{
		m_MoveDir += Dir;
	}

	void SetMoveDir(const Vector3& Dir)
	{
		m_MoveDir = Dir;
	}

	void SetLastUpdateTime(const LARGE_INTEGER Time)
	{
		m_LastUpdateTime = Time;
	}
	const UINT GetObjectID()
	{
		return m_ObjectID;
	}
	const Vector3 GetMoveDir()
	{
		return m_MoveDir;
	}
	const Vector3 GetPos()
	{
		return m_Pos;
	}
	const float GetSpeed()
	{
		return m_MoveSpeed;
	}

	const LARGE_INTEGER GetLastUpdateTime() 
	{
		return m_LastUpdateTime;
	}

	std::mutex& GetMutex()
	{
		return m_Mtx;
	}
	
	void SetLastObjectView(const Vector3& Dir);

	UINT8 GetLastObjectView()
	{
		return m_ObjectView;
	}
	

};

