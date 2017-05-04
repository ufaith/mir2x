/*
 * =====================================================================================
 *
 *       Filename: monoserver.hpp
 *        Created: 02/27/2016 16:45:49
 *  Last Modified: 05/01/2017 16:54:16
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

#include <mutex>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "log.hpp"
#include "message.hpp"
#include "taskhub.hpp"
#include "database.hpp"
#include "uidrecord.hpp"
#include "eventtaskhub.hpp"
#include "monsterginforecord.hpp"

class ServiceCore;
class ServerObject;
class MonoServer final
{
    struct UIDLockRecord
    {
        std::mutex Lock;
        std::unordered_map<uint32_t, ServerObject *> Record;
    };

    private:
        std::mutex m_LogLock;
        std::vector<char> m_LogBuf;

    private:
        ServiceCore *m_ServiceCore;

    private:
        std::atomic<uint32_t> m_GlobalUID;

    private:
        std::array<UIDLockRecord, 17> m_UIDArray;

    private:
        std::chrono::time_point<std::chrono::system_clock> m_StartTime;

    private:
        std::unordered_map<uint32_t, MonsterGInfoRecord> m_MonsterGInfoRecord;

    public:
        void FlushBrowser();

    public:
        const MonsterGInfoRecord &MonsterGInfo(uint32_t nMonsterID) const
        {
            auto pGInfoFind = m_MonsterGInfoRecord.find(nMonsterID);
            if(pGInfoFind == m_MonsterGInfoRecord.end()){
                return MonsterGInfoRecord::Null();
            }

            return pGInfoFind->second;
        }

    public:
        MonoServer();
       ~MonoServer() = default;

    public:
        void ReadHC();

        void Launch();
        void Restart();

    private:
        void RunASIO();
        void CreateServiceCore();
        void CreateDBConnection();

    public:
        void AddLog(const std::array<std::string, 4> &, const char *, ...);

    private:
        bool AddPlayer(uint32_t, uint32_t);

    private:
        bool InitMonsterRace();
        bool InitMonsterItem();
        bool LoadMonsterRecord()
        {
           return InitMonsterRace() && InitMonsterItem();
        }

    private:
        void StartNetwork();

    public:
        // for gui callbacks
        std::vector<uint32_t> GetActiveMapList();
        std::vector<uint32_t> GetValidMonsterList(uint32_t);
        int GetValidMonsterCount(int, int);

    public:
        void AddMonster(uint32_t, uint32_t, int, int);

    public:
        // (uid, instance) managerment
        // for every server object which has an uid we make a global record
        // then given an uid we can retrieve the address quickly without forwarding QUERYADDRESS
        // previously I sent the naked pointer of server object across actors and call GetAddress() for address
        //
        // although GetAddress() is constant which won't alternate actors' internal state
        // this causes the potential issue to call GetAddress() over an actor which is already deleted
        //
        // when a server object is created, it calls LinkUID() to keep a global record
        // and we *only* use EraseUID(ServerObject::UID()) to delete an object allocated on heap
        // this ensures pObject->GetAddress() in GetUIDAddress() always be well-defined
        //
        // requirements:
        //      1. always allcated server object on stack
        //      2. never explicitly call delete pObject, use EraseUID(pObject->UID()) instead
        //      3. never pass naked pointer across actors, pass UID() instead
        //
        // for example:  A : player
        //               B : server map 1
        //               C : server map 2
        //               D : service core
        //
        // now A is at a location at B which should move A to map C, communication with global record:
        // 1. B -> A : you should switch to map C with C_UID
        // 2. A -> C : forward TRYMAPSWITCH to GetUIDAddress(C_UID)
        //
        // without the global record, passing naked pointers
        // 1. B -> A : you should switch to map C, it's address is C_this
        // 2. A -> C : forward TRYMAPSWITCH to C_this->GetAddress(), potential issue here
        //
        // without the global record, passing UID
        // 1. B -> A : you should switch to map C with C_UID
        // 2. A -> D : send QUERYADDRESS to D to get C_Address
        // 3. D -> A : C_Address
        // 4. A -> C : forward TRYMAPSWITCH to C_Address

        // allocate an *unique* uid during current server runtime
        // if an uid is used sometime, it won't be used again anymore
        // don't call MonoServer::GetUID() explicitly, should be called in ServerObject ctor
        uint32_t GetUID();

        // register (uid, instance) pair in global table
        // return false if invlaid arguments or uid has linked to other instance
        // this function is called automatically when constructing the server ojbect
        bool LinkUID(uint32_t, ServerObject *);

        // remove the uid record and its respective instance or do nothing if not exists
        // after this invocation it's guaranteed there shouldn't be an object with given uid
        // 1. it remove the record from the hash table
        // 2. it delete the instance explicitly, so never do delete pObject externally
        void EraseUID(uint32_t);

        // retrieve uid from global tables
        // return a valid uid means it's valid *during the invocation*
        // it could be immediately invalid by EraseUID() from other threads
        UIDRecord GetUIDRecord(uint32_t);

    public:
        uint32_t GetTimeTick()
        {
            return (uint32_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_StartTime).count());
        }
};
