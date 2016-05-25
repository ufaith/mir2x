/*
 * =====================================================================================
 *
 *       Filename: servicecore.hpp
 *        Created: 04/22/2016 17:59:06
 *  Last Modified: 05/24/2016 22:04:59
 *
 *    Description: split monoserver into actor-code and non-actor code
 *                 put all actor code in this class
 *
 *                 TODO & TBD
 *                 everytime when creating a lambda in an actor to use ThreadPN to
 *                 invoke, never use [this, ...] since this will access the internal
 *                 state from another thread
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
#include <vector>
#include <unordered_map>

#include "sessionhub.hpp"
#include "transponder.hpp"

class ServerMap;
class ServiceCore: public Transponder
{
    private:
        uint32_t m_CurrUID;
        SessionHub *m_SessionHub;

    private:
        typedef struct _PlayerRecord {
            uint32_t GUID;
            _PlayerRecord()
                : GUID(0)
            {}
        }PlayerRecord;

        typedef struct _MapRecord {
            uint32_t        MapID;
            ServerMap      *Map;
            Theron::Address PodAddress;
            _MapRecord(uint32_t nMapID = 0)
                : MapID(nMapID)
                , Map(nullptr)
                , PodAddress(Theron::Address::Null())
            {}
        }MapRecord;

        std::unordered_map<uint32_t, MapRecord>    m_MapRecordM;
        std::unordered_map<uint32_t, PlayerRecord> m_PlayerRecordM;


    public:
        ServiceCore();
        virtual ~ServiceCore();

    public:
        void Bind(SessionHub *pHub)
        {
            m_SessionHub = pHub;
            pHub->Bind(GetAddress());
        }

    public:
        void Operate(const MessagePack &, const Theron::Address &);
        void OperateNet(Session*, uint8_t, const uint8_t *, size_t);

    protected:
        bool LoadMap(uint32_t);


    private:
        void On_MPK_LOGIN(const MessagePack &, const Theron::Address &);
        void On_MPK_NETPACKAGE(const MessagePack &, const Theron::Address &);
        void On_MPK_ADDMONSTER(const MessagePack &, const Theron::Address &);
        void On_MPK_PLAYERPHATOM(const MessagePack &, const Theron::Address &);
        void On_MPK_NEWCONNECTION(const MessagePack &, const Theron::Address &);

    private:
        void Net_CM_Login(Session *, uint8_t, const uint8_t *, size_t);
};
