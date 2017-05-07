/*
 * =====================================================================================
 *
 *       Filename: actormessage.hpp
 *        Created: 05/03/2016 13:19:07
 *  Last Modified: 05/06/2017 17:45:59
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
#include <cstdint>
enum MessagePackType: int
{
    MPK_NONE = 0,
    MPK_OK,
    MPK_ERROR,
    MPK_UID,
    MPK_HI,
    MPK_PING,
    MPK_LOGIN,
    MPK_METRONOME,
    MPK_TRYMOVE,
    MPK_TRYLEAVE,
    MPK_TRYSPACEMOVE,
    MPK_LOGINOK,
    MPK_ADDRESS,
    MPK_LOGINQUERYDB,
    MPK_NETPACKAGE,
    MPK_ADDCHAROBJECT,
    MPK_BINDSESSION,
    MPK_ACTION,
    MPK_STATE,
    MPK_QUERYMONSTERGINFO,
    MPK_PULLCOINFO,
    MPK_NEWCONNECTION,
    MPK_QUERYMAPLIST,
    MPK_MAPLIST,
    MPK_MAPSWITCH,
    MPK_MAPSWITCHOK,
    MPK_TRYMAPSWITCH,
    MPK_QUERYMAPUID,
    MPK_QUERYLOCATION,
    MPK_LOCATION,
    MPK_PATHFIND,
    MPK_PATHFINDOK,
    MPK_ATTACK,
};

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    int X;
    int Y;
}AMTryLeave;

typedef union
{
    uint8_t Type;

    struct _Common
    {
        uint8_t Type;

        uint32_t MapID;
        int X;
        int Y;
    }Common;

    struct _Monster
    {
        struct _Common _MemoryAlign;
        uint32_t MonsterID;
    }Monster;

    struct _Player
    {
        struct _Common _MemoryAlign;
        uint32_t DBID;
        uint32_t JobID;
        int Level;
        int Direction;
        uint32_t SessionID;
    }Player;

    struct _NPC
    {
        struct _Common _MemoryAlign;
        uint32_t NPCID;
    }NPC;
}AMAddCharObject;

typedef struct
{
    uint32_t DBID;
    uint32_t UID;
    uint32_t SID;
    uint32_t MapID;
    uint64_t Key;

    int X;
    int Y;
}AMLogin;

typedef struct
{
    void *This;

    uint32_t MapID;
    uint32_t UID;

    int X;
    int Y;

    int CurrX;
    int CurrY;
}AMTrySpaceMove;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    int X;
    int Y;

    int EndX;
    int EndY;
}AMTryMove;

typedef struct
{
    uint32_t SessionID;

    uint32_t DBID;
    uint32_t MapID;
    int      MapX;
    int      MapY;
    int      Level;
    int      JobID;
    int      Direction;
}AMLoginQueryDB;

typedef struct
{
    uint32_t SessionID;
    uint8_t  Type;

    const uint8_t *Data;
    size_t DataLen;
}AMNetPackage;

typedef struct
{
    uint32_t SessionID;
}AMBindSession;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    int X;
    int Y;
}AMState;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    int Action;
    int ActionParam;

    int Speed;
    int Direction;

    int X;
    int Y;
    
    int EndX;
    int EndY;

    uint32_t ID;
}AMAction;

typedef struct
{
    uint32_t MonsterID;
    int      LookIDN;
    uint32_t SessionID;
}AMQueryMonsterGInfo;

typedef struct
{
    uint32_t SessionID;
}AMPullCOInfo;

typedef struct
{
    uint32_t SessionID;
}AMNewConnection;

typedef struct
{
    uint32_t MapList[256];
}AMMapList;

typedef struct
{
    uint32_t MapID;
    uint32_t UID;
}AMMapSwitch;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;
    uint32_t MapUID;

    int X;
    int Y;
}AMTryMapSwitch;

typedef struct
{
    uint32_t MapID;
}AMQueryMapUID;

typedef struct
{
    uint32_t UID;
}AMUID;

typedef struct
{
    void *Data;

    int X;
    int Y;
}AMMapSwitchOK;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;
}AMQueryLocation;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;
    
    int X;
    int Y;
}AMLocation;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    bool CheckCO;

    int X;
    int Y;
    int EndX;
    int EndY;
}AMPathFind;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    struct _Point
    {
        int X;
        int Y;
    }Point[2];
}AMPathFindOK;

typedef struct
{
    uint32_t UID;
    uint32_t MapID;

    int X;
    int Y;
}AMAttack;
