/*
 * =====================================================================================
 *
 *       Filename: monster.hpp
 *        Created: 04/10/2016 02:32:45 AM
 *  Last Modified: 05/16/2017 18:19:33
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
#include <functional>
#include "charobject.hpp"

enum MonsterType: uint32_t
{
    MONSTER_DEER,
};

typedef struct stMONSTERITEMINFO
{
    int     MonsterIndex;
    int     Type;
    int     Chance;
    int     Count;

    stMONSTERITEMINFO(int nMonsterIndex = -1)
        : MonsterIndex(nMonsterIndex)
    {}
}MONSTERITEMINFO;

typedef struct stMONSTERRACEINFO
{
    int     Index;
    int     Race;
    int     LID;
    int     Undead;
    int     Level;
    int     HP;
    int     MP;
    int     AC;
    int     MAC;
    int     DC;
    int     AttackSpead;
    int     WalkSpead;
    int     Spead;
    int     Hit;
    int     ViewRange;
    int     RaceIndex;
    int     Exp;
    int     Escape;
    int     Water;
    int     Fire;
    int     Wind;
    int     Light;
    int     Earth;

    std::string Name;
    std::vector<MONSTERITEMINFO> ItemV;

    stMONSTERRACEINFO(int nIndex = -1)
        : Index(nIndex)
        , Name("")
    {}
}MONSTERRACEINFO;

class Monster: public CharObject
{
    protected:
        const uint32_t m_MonsterID;

    public:
        Monster(uint32_t,               // monster id
                ServiceCore *,          // service core
                ServerMap *,            // server map
                int,                    // map x
                int,                    // map y
                int,                    // direction
                uint8_t);               // life cycle state
       ~Monster() = default;

    public:
       uint32_t MonsterID() const
       {
           return m_MonsterID;
       }

    public:
        void SearchViewRange();
        bool Update();

    protected:
        bool RandomMove();
        bool TrackAttack();

    protected:
        bool TrackUID(uint32_t);
        bool AttackUID(uint32_t, int);

    protected:
        bool DCValid(int, bool);
        bool InRange(int, int, int);

    protected:
        void AddTarget(uint32_t);

    protected:
        int GetAttackPower(int);

    protected:
        bool ParseNewAction(const NoticeNode &, bool);
        bool ParseNewNotice(const NoticeNode &, bool);

    private:
        void On_MPK_ACTION(const MessagePack &, const Theron::Address &);
        void On_MPK_METRONOME(const MessagePack &, const Theron::Address &);
        void On_MPK_MAPSWITCH(const MessagePack &, const Theron::Address &);
        void On_MPK_PULLCOINFO(const MessagePack &, const Theron::Address &);
        void On_MPK_QUERYLOCATION(const MessagePack &, const Theron::Address &);

    protected:
        void Operate(const MessagePack &, const Theron::Address &);

    protected:
        void ReportCORecord(uint32_t);

    protected:
        bool CanMove();
        bool CanAttack();
};
