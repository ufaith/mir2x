/*
 * =====================================================================================
 *
 *       Filename: monster.cpp
 *        Created: 04/07/2016 03:48:41 AM
 *  Last Modified: 05/18/2017 23:18:41
 *
 *    Description: 
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "motion.hpp"
#include "netpod.hpp"
#include "dbconst.hpp"
#include "monster.hpp"
#include "actorpod.hpp"
#include "mathfunc.hpp"
#include "memorypn.hpp"
#include "randompick.hpp"
#include "monoserver.hpp"
#include "messagepack.hpp"
#include "protocoldef.hpp"

Monster::Monster(uint32_t   nMonsterID,
        ServiceCore        *pServiceCore,
        ServerMap          *pServerMap,
        int                 nMapX,
        int                 nMapY,
        int                 nDirection,
        uint8_t             nLifeState)
    : CharObject(pServiceCore, pServerMap, nMapX, nMapY, nDirection, nLifeState)
    , m_MonsterID(nMonsterID)
{
    auto fnRegisterClass = [this]() -> void {
        if(!RegisterClass<Monster, CharObject>()){
            extern MonoServer *g_MonoServer;
            g_MonoServer->AddLog(LOGTYPE_WARNING, "Class registration for <Monster, CharObject> failed");
            g_MonoServer->Restart();
        }
    };
    static std::once_flag stFlag;
    std::call_once(stFlag, fnRegisterClass);

    // set attack mode
    SetState(STATE_ATTACKMODE, STATE_ATTACKMODE_NORMAL);
}

bool Monster::RandomMove()
{
    if(CanMove()){
        // 1. try move ahead
        //    would fail if next grid is not walkable
        {
            int nX = -1;
            int nY = -1;
            if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
                if(true
                        && (rstMR.WalkStep > 0)
                        && (OneStepReach(Direction(), rstMR.WalkStep, &nX, &nY) == rstMR.WalkStep)){
                    switch(rstMR.WalkStep){
                        case 1  : return RequestMove(MOTION_WALK, nX, nY, true, [](){}, [](){});
                        case 2  : return RequestMove(MOTION_RUN,  nX, nY, true, [](){}, [](){});
                        default : break;
                    }

                    extern MonoServer *g_MonoServer;
                    g_MonoServer->AddLog(LOGTYPE_WARNING, "Invalid monster record");
                    return false;
                }
            }
        }

        // 2. if current direction leads to a *impossible* place
        //    then randomly take a new direction and try
        {
            static const int nDirV[] = {
                DIR_UP,
                DIR_UPRIGHT,
                DIR_RIGHT,
                DIR_DOWNRIGHT,
                DIR_DOWN,
                DIR_DOWNLEFT,
                DIR_LEFT,
                DIR_UPLEFT,
            };

            auto nDirCount = (int)(sizeof(nDirV) / sizeof(nDirV[0]));
            auto nDirStart = (int)(std::rand() % nDirCount);

            for(int nIndex = 0; nIndex < nDirCount; ++nIndex){
                auto nDirection = nDirV[(nDirStart + nIndex) % nDirCount];
                if(Direction() != nDirection){
                    int nX = -1;
                    int nY = -1;
                    if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
                        if(true
                                && (rstMR.WalkStep > 0)
                                && (OneStepReach(nDirection, rstMR.WalkStep, &nX, &nY) == rstMR.WalkStep)){
                            // TODO
                            // current direction is possible for next move
                            // don't do turn and motion now
                            // report the turn and do motion (by chance) in next update
                            m_Direction = nDirection;
                            DispatchAction({ACTION_STAND, 0, Direction(), X(), Y(), MapID()});

                            // TODO
                            // we won't do ReportStand() for monster
                            // monster's moving is only driven by server currently
                            return true;
                        }
                    }
                }
            }
        }

        // 3. more motion method
        //    ....
    }
    return false;
}

bool Monster::AttackUID(uint32_t nUID, int nDC)
{
    if(CanAttack()){
        extern MonoServer *g_MonoServer;
        if(auto stRecord = g_MonoServer->GetUIDRecord(nUID)){
            if(DCValid(nDC, true)){
                auto fnQueryLocationOK = [this, nDC, stRecord](int nX, int nY) -> bool {

                    // if we get inside current block, we should release the attack lock
                    // it can be released immediately if location retrieve succeeds without querying
                    m_AttackLock = false;

                    switch(nDC){
                        case DC_PHY_PLAIN:
                            {
                                switch(LDistance2(X(), Y(), nX, nY)){
                                    case 0:
                                    case 1:
                                    case 2:
                                        {
                                            static const int nDirV[][3] = {
                                                {DIR_UPLEFT,   DIR_UP,   DIR_UPRIGHT  },
                                                {DIR_LEFT,     DIR_NONE, DIR_RIGHT    },
                                                {DIR_DOWNLEFT, DIR_DOWN, DIR_DOWNRIGHT}};

                                            int nDX = nX - X() + 1;
                                            int nDY = nY - Y() + 1;

                                            m_Direction = nDirV[nDY][nDX];

                                            // 1. dispatch action to all
                                            DispatchAction({ACTION_ATTACK, DC_PHY_PLAIN, Direction(), X(), Y(), MapID()});

                                            extern MonoServer *g_MonoServer;
                                            m_LastAttackTime = g_MonoServer->GetTimeTick();

                                            // 2. send attack message to target
                                            //    target can ignore this message directly
                                            AMAttack stAMA;
                                            stAMA.UID   = UID();
                                            stAMA.MapID = MapID();

                                            stAMA.Mode  = EC_NONE;
                                            stAMA.Power = GetAttackPower(DC_PHY_PLAIN);

                                            stAMA.X = X();
                                            stAMA.Y = Y();
                                            return m_ActorPod->Forward({MPK_ATTACK, stAMA}, stRecord.Address);
                                        }
                                    default:
                                        {
                                            // TODO
                                            // one issue is evertime we do AttackUID() || TrackUID()
                                            // then could be a case everytime we schedule an attack operation
                                            // but actually this attack can't be done successfully
                                            // then we don't do AttackUID() nor TrackUID()
                                            TrackUID(stRecord.UID);
                                            return false;
                                        }
                                }
                            }
                        case DC_MAG_FIRE:
                            {
                                return false;
                            }
                        default:
                            {
                                break;
                            }
                    }
                    return false;
                };

                // retrieving could schedule an location query
                // before the response we can't allow any attack request

                m_AttackLock = true;
                return RetrieveLocation(nUID, fnQueryLocationOK);
            }
        }
    }
    return false;
}

bool Monster::TrackUID(uint32_t nUID)
{
    if(true
            && nUID
            && CanMove()){
        extern MonoServer *g_MonoServer;
        if(auto stRecord = g_MonoServer->GetUIDRecord(nUID)){
            return RetrieveLocation(nUID, [this](int nX, int nY){
                switch(LDistance2(nX, nY, X(), Y())){
                    case 0:
                    case 1:
                        {
                            return true;
                        }
                    default:
                        {
                            if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
                                switch(rstMR.WalkStep){
                                    case 1  : return MoveOneStep(MOTION_WALK, nX, nY);
                                    case 2  : return MoveOneStep(MOTION_RUN,  nX, nY);
                                    default : break;
                                }
                            }
                            return false;
                        }
                }
            });
        }
    }
    return false;
}

bool Monster::TrackAttack()
{
    while(!m_TargetQ.empty()){
        // 1. check if time expired
        //    delete if it has a long time not (able to) track it
        extern MonoServer *g_MonoServer;
        if(m_TargetQ.front().ActiveTime + 60 * 1000 <= g_MonoServer->GetTimeTick()){
            m_TargetQ.pop_front();
            continue;
        }

        // 2. ok not expired
        //    check if current UID is valid
        //    track and record the track time if succeed
        extern MonoServer *g_MonoServer;
        if(auto stRecord = g_MonoServer->GetUIDRecord(m_TargetQ.front().UID)){
            // 1. try to attack uid
            if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
                for(auto nDC: rstMR.DCList){
                    if(AttackUID(stRecord.UID, nDC)){
                        extern MonoServer *g_MonoServer;
                        m_TargetQ.front().ActiveTime = g_MonoServer->GetTimeTick();
                        return true;
                    }
                }
            }

            // 2. try to track uid
            if(TrackUID(stRecord.UID)){
                m_TargetQ.front().ActiveTime = g_MonoServer->GetTimeTick();
                return true;
            }

            // 3. not a proper target
            //    try next one
            m_TargetQ.push_back(m_TargetQ.front());
            m_TargetQ.pop_front();
            continue;
        }else{
            // 3. not valid even
            //    delete directly and try next one
            m_TargetQ.pop_front();
            continue;
        }
    }

    // no operation performed or issued
    return false;
}

bool Monster::Update()
{
    if(TrackAttack()){ return true; }
    if(RandomMove ()){ return true; }

    return false;
}

void Monster::Operate(const MessagePack &rstMPK, const Theron::Address &rstAddress)
{
    switch(rstMPK.Type()){
        case MPK_METRONOME:
            {
                On_MPK_METRONOME(rstMPK, rstAddress);
                break;
            }
        case MPK_ACTION:
            {
                On_MPK_ACTION(rstMPK, rstAddress);
                break;
            }
        case MPK_ATTACK:
            {
                On_MPK_ATTACK(rstMPK, rstAddress);
                break;
            }
        case MPK_MAPSWITCH:
            {
                On_MPK_MAPSWITCH(rstMPK, rstAddress);
                break;
            }
        case MPK_QUERYLOCATION:
            {
                On_MPK_QUERYLOCATION(rstMPK, rstAddress);
                break;
            }
        case MPK_PULLCOINFO:
            {
                On_MPK_PULLCOINFO(rstMPK, rstAddress);
                break;
            }
        default:
            {
                extern MonoServer *g_MonoServer;
                g_MonoServer->AddLog(LOGTYPE_WARNING, "Unsupported message: %s", rstMPK.Name());
                g_MonoServer->Restart();
                break;
            }
    }
}

void Monster::SearchViewRange()
{
}

void Monster::ReportCORecord(uint32_t nSessionID)
{
    if(nSessionID){
        SMCORecord stSMCOR;
        // TODO: don't use OBJECT_MONSTER, we need translation
        //       rule of communication, the sender is responsible to translate

        // 1. set type
        stSMCOR.Type = CREATURE_MONSTER;

        // 2. set common info
        stSMCOR.Common.UID   = UID();
        stSMCOR.Common.MapID = MapID();

        stSMCOR.Common.Action      = ACTION_STAND;
        stSMCOR.Common.ActionParam = 0;
        stSMCOR.Common.Speed       = 0;
        stSMCOR.Common.Direction   = Direction();

        stSMCOR.Common.X    = X();
        stSMCOR.Common.Y    = Y();
        stSMCOR.Common.EndX = X();
        stSMCOR.Common.EndY = Y();

        // 3. set specified info
        stSMCOR.Monster.MonsterID = m_MonsterID;

        extern NetPodN *g_NetPodN;
        g_NetPodN->Send(nSessionID, SM_CORECORD, stSMCOR);
    }else{
        extern MonoServer *g_MonoServer;
        g_MonoServer->AddLog(LOGTYPE_WARNING, "invalid session id");
        g_MonoServer->Restart();
    }
}

bool Monster::InRange(int nRangeType, int nX, int nY)
{
    if(true
            && m_Map
            && m_Map->ValidC(nX, nY)){
        switch(nRangeType){
            case RANGE_VISIBLE:
                {
                    return LDistance2(X(), Y(), nX, nY) < 20 * 20;
                }
            case RANGE_ATTACK:
                {
                    // inside this range
                    // monster will decide to make an attack
                    return LDistance2(X(), Y(), nX, nY) < 10 * 10;
                }
        }
    }
    return false;
}

int Monster::GetAttackPower(int nAttackParam)
{
    if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
        switch(nAttackParam){
            case DC_PHY_PLAIN:
                {
                    return rstMR.DC + std::rand() % (1 + std::max<int>(rstMR.DCMax - rstMR.DC, 0));
                }
            case DC_MAG_FIRE:
                {
                    return rstMR.MC + std::rand() % (1 + std::max<int>(rstMR.MCMax - rstMR.MC, 0));
                }
            default:
                {
                    break;
                }
        }
    }
    return -1;
}

bool Monster::CanMove()
{
    if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
        if(CharObject::CanMove()){
            extern MonoServer *g_MonoServer;
            return m_LastMoveTime + rstMR.WalkWait <= g_MonoServer->GetTimeTick();
        }
    }
    return false;
}

bool Monster::CanAttack()
{
    if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
        if(CharObject::CanAttack()){
            extern MonoServer *g_MonoServer;
            return m_LastAttackTime + rstMR.AttackWait <= g_MonoServer->GetTimeTick();
        }
    }
    return false;
}

bool Monster::DCValid(int nDC, bool bCheck)
{
    if(auto &rstMR = DB_MONSTERRECORD(MonsterID())){
        if(std::find(rstMR.DCList.begin(), rstMR.DCList.end(), nDC) != rstMR.DCList.end()){
            if(bCheck){
                if(auto &rstDCR = DB_DCRECORD(nDC)){
                    if(m_HP < rstDCR.HP){ return false; }
                    if(m_MP < rstDCR.MP){ return false; }
                }
            }
            return true;
        }
    }
    return false;
}

void Monster::AddTarget(uint32_t nUID)
{
    if(std::find_if(m_TargetQ.begin(), m_TargetQ.end(), [nUID](TargetRecord &rstRecord){
        if(rstRecord.UID == nUID){
            extern MonoServer *g_MonoServer;
            rstRecord.ActiveTime = g_MonoServer->GetTimeTick();
            return true;
        }else{ return false; }
    }) == m_TargetQ.end()){
        extern MonoServer *g_MonoServer;
        m_TargetQ.emplace_back(nUID, g_MonoServer->GetTimeTick());
    }
}
