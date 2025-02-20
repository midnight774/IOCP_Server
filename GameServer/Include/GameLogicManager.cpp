
#include "GameLogicManager.h"
#include "GamePacketManager.h"
#include "ClientSession.h"
#include "PacketSerializer.h"
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

            UpdateCharacterMove();
            UpdateCharacterAttack();
            
        }

        std::unique_lock<std::mutex> Lock(m_Mtx);
        m_UpdateAvailable.wait_for(Lock, std::chrono::milliseconds(100));
    }
}

void CGameLogicManager::UpdateCharacterMove()
{
    auto iter = m_mapUpdateClient.begin();
    auto iterEnd = m_mapUpdateClient.end();
    int Stride = sizeof(UINT8);
    Stride += sizeof(int);
    int Cnt = 0;

    LARGE_INTEGER   Time = {};
    QueryPerformanceCounter(&Time);
    std::vector<CharacterMoveData> vecMoveData;
    vecMoveData.reserve(m_mapUpdateClient.size());

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

        //Serializer에 넘겨줄 데이터 담는다.
        vecMoveData.emplace_back(Time, ObjInfo->GetObjectID(), 0, MovePos.x, MovePos.y, MoveDir == Vector3(0.f, 0.f, 0.f));

        if (!(MoveDir == Vector3(0.f, 0.f, 0.f)))
        {
            ObjInfo->SetLastObjectView(MoveDir);
        }

        ++Cnt;

        if (MoveDir == Vector3(0.f, 0.f, 0.f))
            iter = m_mapUpdateClient.erase(iter);

        else
            ++iter;
    }

    if (Cnt != 0)
    {
        char DataToSend[1024];
        memset(DataToSend, 0, 1024);
        int DataSize = 0;
        CPacketSerializer::SerializeCharacterMoves(vecMoveData, DataToSend, DataSize);
        CGamePacketManager::GetInst()->EchoGameData(DataToSend, DataSize);
    }
}

void CGameLogicManager::UpdateCharacterAttack()
{
    std::vector<CharacterAttackData> vecCharacterAtk;
    vecCharacterAtk.reserve(m_qAttackingCharacter.size());

    while (!m_qAttackingCharacter.empty())
    {
        std::shared_ptr<CClientSession> CurSession = m_qAttackingCharacter.front();
        m_qAttackingCharacter.pop();
        std::shared_ptr<CGameObjectInfo> Info = CurSession->m_ClientCharacterInfo;
      
        Vector3 Pos = Info->GetPos();
        UINT8 Dir = Info->GetLastObjectView();
        vecCharacterAtk.emplace_back(Info->GetObjectID(), Pos.x, Pos.y, Dir);
    }

    if (!vecCharacterAtk.empty())
    {
        char DataToSend[1024];
        memset(DataToSend, 0, 1024);
        int DataSize = 0;
        CPacketSerializer::SerializeCharacterAttack(vecCharacterAtk, DataToSend, DataSize);
        CGamePacketManager::GetInst()->EchoGameData(DataToSend, DataSize);
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

void CGameLogicManager::PushAttackingCharacter(std::shared_ptr<CClientSession> pSession)
{
    //공격 명령이 들어오면 이동 중단
    RemoveUpdateClient(pSession);

    std::lock_guard<std::mutex> Lock(m_Mtx);
    pSession->m_ClientCharacterInfo->SetMoveDir(Vector3(0.f, 0.f, 0.f));

    m_qAttackingCharacter.push(pSession);
}

const int CGameLogicManager::GetLeastWorkThreadIdx()
{
    return 0;
}
