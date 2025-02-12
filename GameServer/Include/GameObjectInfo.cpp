#include "GameObjectInfo.h"

CGameObjectInfo::CGameObjectInfo()
{
}

CGameObjectInfo::~CGameObjectInfo()
{
}

void CGameObjectInfo::SetLastObjectView(const Vector3& Dir)
{
       /*  Front = 0,
        LeftFront,
        Left,
        LeftBack,
        Back,
        RightBack,
        Right,
        RightFront,*/

    if (Dir == Vector3(0.f, -1.f, 0.f))
    {
        m_ObjectView = 0;
    }
    else if (Dir == Vector3(-1.f, -1.f, 0.f))
    {
        m_ObjectView = 1;
    }
    else if (Dir == Vector3(-1.f, 0.f, 0.f))
    {
        m_ObjectView = 2;
    }
    else if (Dir == Vector3(-1.f, 1.f, 0.f))
    {
        m_ObjectView = 3;
    }
    else if (Dir == Vector3(0.f, 1.f, 0.f))
    {
        m_ObjectView = 4;
    }
    else if (Dir == Vector3(1.f, 1.f, 0.f))
    {
        m_ObjectView = 5;
    }
    else if (Dir == Vector3(1.f, 0.f, 0.f))
    {
        m_ObjectView = 6;
    }
    else if (Dir == Vector3(1.f, -1.f, 0.f))
    {
        m_ObjectView = 7;
    }
}
