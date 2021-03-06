/*
 * =====================================================================================
 *
 *       Filename: charobject.hpp
 *        Created: 04/10/2016 12:05:22
 *  Last Modified: 05/16/2017 23:05:21
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
#pragma once

#include <list>
#include <deque>
#include <vector>

#include "servermap.hpp"
#include "actionnode.hpp"
#include "noticenode.hpp"
#include "servicecore.hpp"
#include "protocoldef.hpp"
#include "activeobject.hpp"

enum _FriendType: uint8_t
{
    FRIEND_HUMAN,
    FRIEND_ANIMAL,
    FRIEND_NEUTRAL,
};

enum _RangeType: uint8_t
{
    RANGE_VIEW,
    RANGE_MAP,
    RANGE_SERVER,

    RANGE_VISIBLE,
    RANGE_ATTACK,
    RANGE_TRACETARGET,
};

#pragma pack(push, 1)
typedef struct
{
    uint8_t     Level;
    uint16_t    HP;
    uint16_t    MP;
    uint16_t    MaxHP;
    uint16_t    MaxMP;
    uint16_t    Weight;
    uint16_t    MaxWeight;
    uint32_t    Exp;
    uint32_t    MaxExp;

    uint8_t     WearWeight;
    uint8_t     MaxWearWeight;
    uint8_t     HandWeight;
    uint8_t     MaxHandWeight;

    uint16_t    DC;
    uint16_t    MC;
    uint16_t    SC;
    uint16_t    AC;
    uint16_t    MAC;

    uint16_t    Water;
    uint16_t    Fire;
    uint16_t    Wind;
    uint16_t    Light;
    uint16_t    Earth;
}OBJECTABILITY;

typedef struct
{
    uint16_t    HP;
    uint16_t    MP;
    uint16_t    HIT;
    uint16_t    SPEED;
    uint16_t    AC;
    uint16_t    MAC;
    uint16_t    DC;
    uint16_t    MC;
    uint16_t    SC;
    uint16_t    AntiPoison;
    uint16_t    PoisonRecover;
    uint16_t    HealthRecover;
    uint16_t    SpellRecover;
    uint16_t    AntiMagic;
    uint8_t     Luck;
    uint8_t     UnLuck;
    uint8_t     WeaponStrong;
    uint16_t    HitSpeed;
}OBJECTADDABILITY;
#pragma pack(pop)

class CharObject: public ActiveObject
{
    protected:
        struct COLocation
        {
            uint32_t UID;
            uint32_t MapID;
            uint32_t RecordTime;

            int X;
            int Y;

            COLocation()
                : UID(0)
                , MapID(0)
                , RecordTime(0)
                , X(-1)
                , Y(-1)
            {}
        };

        struct TargetRecord
        {
            uint32_t UID;
            uint32_t ActiveTime;

            TargetRecord(uint32_t nUID, uint32_t nActiveTime)
                : UID(nUID)
                , ActiveTime(nActiveTime)
            {}
        };

    protected:
        ServiceCore *m_ServiceCore;
        ServerMap   *m_Map;

    protected:
        std::unordered_map<uint32_t, ServerMap *> m_MapCache;
        std::unordered_map<uint32_t, COLocation > m_LocationRecord;

    protected:
        int m_CurrX;
        int m_CurrY;
        int m_Direction;

    protected:
        int m_HP;
        int m_MP;

    protected:
        bool     m_MoveLock;
        bool     m_AttackLock;
        uint32_t m_LastMoveTime;
        uint32_t m_LastAttackTime;

    protected:
        std::deque<TargetRecord> m_TargetQ;

    protected:
        OBJECTABILITY       m_Ability;
        OBJECTABILITY       m_WAbility;
        OBJECTADDABILITY    m_AddAbility;

    public:
        CharObject(ServiceCore *,       // service core
                ServerMap *,            // server map
                int,                    // map x
                int,                    // map y
                int,                    // direction
                uint8_t);               // life cycle state
       ~CharObject() = default;

    public:
        bool Active()
        {
            if(GetState(STATE_DEAD   )){ return false; }
            if(GetState(STATE_PHANTOM)){ return false; }

            return true;
        }

    protected:
        int X()
        {
            return m_CurrX;
        }

        int Y()
        {   
            return m_CurrY;
        }

        int Direction()
        {
            return m_Direction;
        }

        uint32_t MapID()
        {
            return m_Map ? m_Map->ID() : 0;
        }

    public:
        uint8_t GetBack()
        {
            switch (m_Direction){
                case DIR_UP       : return DIR_DOWN;
                case DIR_DOWN     : return DIR_UP;
                case DIR_LEFT     : return DIR_RIGHT;
                case DIR_RIGHT    : return DIR_LEFT;
                case DIR_UPLEFT   : return DIR_DOWNRIGHT;
                case DIR_UPRIGHT  : return DIR_DOWNLEFT;
                case DIR_DOWNLEFT : return DIR_UPRIGHT;
                case DIR_DOWNRIGHT: return DIR_UPLEFT;
                default           : return DIR_NONE;
            }
        }

    public:
        virtual bool Update() = 0;
        virtual bool InRange(int, int, int) = 0;

    public:
        bool NextLocation(int *, int *, int, int);
        bool NextLocation(int *pX, int *pY, int nDistance)
        {
            return NextLocation(pX, pY, Direction(), nDistance);
        }

    public:
        Theron::Address Activate();

    protected:
        virtual void ReportCORecord(uint32_t) = 0;

    protected:
        void DispatchAction(const ActionNode &);
        void DispatchNotice(const NoticeNode &);

    protected:
        virtual int OneStepReach(int, int, int *, int *);

    protected:
        virtual bool CanMove();
        virtual bool MoveOneStep(int, int, int);
        virtual bool RetrieveLocation(uint32_t, std::function<void(int, int)>);
        virtual bool RequestMove(int, int, int, bool, std::function<void()>, std::function<void()>);

    protected:
        virtual bool CanAttack();
        virtual bool StruckDamage();

    protected:
        virtual int GetAttackPower(int nAttackMode) = 0;

    protected:
        virtual bool Disappear();

    protected:
        virtual bool GoDie();
        virtual bool GoGhost();
        virtual bool GoSuicide();
};
