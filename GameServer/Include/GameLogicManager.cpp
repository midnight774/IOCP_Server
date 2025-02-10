
#include "GameLogicManager.h"
#include "GamePacketManager.h"
#include "ClientSession.h"
#include "GameObjectInfo.h"

DEFINITION_SINGLETON(CGameLogicManager);

CGameLogicManager::CGameLogicManager()
{

}

CGameLogicManager::~CGameLogicManager()
{
    m_mapUpdateClient.clear();
    m_GameLogicThread.reset();
}

bool CGameLogicManager::Init()
{
    QueryPerformanceFrequency(&m_Second);
    m_GameLogicThread = std::make_shared<std::thread>(&CGameLogicManager::UpdateCharacter, m_pInst);
    
    m_IsStop = false;

    return true;
}

void CGameLogicManager::InitThreadPool(int ThreadCnt)
{
}


void CGameLogicManager::UpdateCharacter()
{
    while (!m_IsStop)
    {
        {
            std::lock_guard<std::mutex> LockGuard(m_Mtx);

            char DataToSend[512] = {};

            auto iter = m_mapUpdateClient.begin();
            auto iterEnd = m_mapUpdateClient.end();
            int Size = sizeof(UINT8);
            Size += sizeof(int);
            int Cnt = 0;

            LARGE_INTEGER   Time = {};
            QueryPerformanceCounter(&Time);

            for (; iter != iterEnd;)
            {
                std::lock_guard<std::mutex> Lock(iter->second->m_Mutex);

                std::shared_ptr<CGameObjectInfo> ObjInfo = iter->second->m_ClientCharacterInfo;
                Vector3 CurPos = ObjInfo->GetPos();
                Vector3 MoveDir = ObjInfo->GetMoveDir();

                LARGE_INTEGER LastUpdateTime = ObjInfo->GetLastUpdateTime();
                float DeltaTime = (Time.QuadPart - LastUpdateTime.QuadPart) / (float)m_Second.QuadPart;

                Vector3 MovePos = CurPos + MoveDir * 200.f * DeltaTime;
                ObjInfo->SetPos(MovePos);
                ObjInfo->SetLastUpdateTime(Time);
                
                CharacterMoveData Data;
                memset(&Data, 0, sizeof(CharacterMoveData));
                Data.Time = Time;
                Data.Dir = 0;
                Data.PosX = MovePos.x;
                Data.PosY = MovePos.y;
                Data.ObjectID = ObjInfo->GetObjectID();
                Data.IsEnd = MoveDir == Vector3(0.f, 0.f, 0.f) ? true : false;
                
                memcpy(DataToSend + Size, &Data, sizeof(CharacterMoveData));
                Size += sizeof(CharacterMoveData);
                ++Cnt;

                if (Data.IsEnd)
                    iter = m_mapUpdateClient.erase(iter);

                else
                    ++iter;
            }

            if(Cnt != 0)
            {
                Packet_Type Type = Packet_Type::CharacterMove;
                memcpy(DataToSend, &Type, sizeof(UINT8));
                memcpy(DataToSend + sizeof(UINT8), &Cnt, sizeof(int));
                CGamePacketManager::GetInst()->EchoGameData(DataToSend, Size);
            }
        }

        std::unique_lock<std::mutex> Lock(m_Mtx);
        m_UpdateAvailable.wait_for(Lock, std::chrono::milliseconds(100));
    }
}

std::shared_ptr<CClientSession> CGameLogicManager::FindClient(std::shared_ptr<CClientSession> pSession)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    auto iter = m_mapUpdateClient.find(pSession.get());

    if (iter == m_mapUpdateClient.end())
        return nullptr;

    return iter->second;
}

void CGameLogicManager::AddUpdateClient(std::shared_ptr<CClientSession> pSession)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    m_mapUpdateClient.insert(std::pair<CClientSession*, std::shared_ptr<CClientSession>>(pSession.get(), pSession));
}

void CGameLogicManager::RemoveUpdateClient(std::shared_ptr<CClientSession> pSession)
{
    std::lock_guard<std::mutex> Lock(m_Mtx);
    m_mapUpdateClient.erase(pSession.get());
}

const int CGameLogicManager::GetLeastWorkThreadIdx()
{
    return 0;
}
