/*
 * =====================================================================================
 *
 *       Filename: servicecorenet.cpp
 *        Created: 05/20/2016 17:09:13
 *  Last Modified: 03/27/2017 16:57:09
 *
 *    Description: interaction btw NetPod and ServiceCore
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
#include "dbpod.hpp"
#include "threadpn.hpp"
#include "monoserver.hpp"
#include "servicecore.hpp"

void ServiceCore::Net_CM_Login(uint32_t nSessionID, uint8_t, const uint8_t *pData, size_t)
{
    // message structure:  ID\0PWD\0, copy it out since pData is temperal
    // TODO: client can put more info following ID/PWD and we will analyze it here
    std::string szID   = (char *)pData;    pData += (1 + std::strlen((char *)pData));
    std::string szPWD  = (char *)pData; // pData += (1 + std::strlen((char *)pData));

    // don't block ServiceCore too much, so we put rest of it 
    // in the thread pool since it's db query and slow
    //
    // here we put pSession, but how about this session has been killed
    // when this lambda invoked
    auto fnDBOperation = [nSessionID, stSCAddr = GetAddress(), szID, szPWD](){
        extern DBPodN *g_DBPodN;
        extern MonoServer *g_MonoServer;

        g_MonoServer->AddLog(LOGTYPE_INFO, "Login requested: (%s:%s)", szID.c_str(), szPWD.c_str());
        auto pDBHDR = g_DBPodN->CreateDBHDR();

        if(!pDBHDR->Execute("select fld_id from tbl_account where fld_account = '%s' and fld_password = '%s'", szID.c_str(), szPWD.c_str())){
            g_MonoServer->AddLog(LOGTYPE_WARNING, "SQL ERROR: (%d: %s)", pDBHDR->ErrorID(), pDBHDR->ErrorInfo());
            SyncDriver().Forward({SM_LOGINFAIL, nSessionID}, stSCAddr);
            return;
        }

        if(pDBHDR->RowCount() < 1){
            g_MonoServer->AddLog(LOGTYPE_INFO, "can't find account: (%s:%s)", szID.c_str(), szPWD.c_str());
            SyncDriver().Forward({SM_LOGINFAIL, nSessionID}, stSCAddr);
            return;
        }

        pDBHDR->Fetch();
        // you can put another lambda here and put it in g_TaskHub
        // but doesn't make sense since this function is already slow
        int nID = std::atoi(pDBHDR->Get("fld_id"));
        if(!pDBHDR->Execute("select * from mir2x.tbl_guid where fld_id = %d", nID)){
            g_MonoServer->AddLog(LOGTYPE_WARNING, "SQL ERROR: (%d: %s)", pDBHDR->ErrorID(), pDBHDR->ErrorInfo());
            SyncDriver().Forward({SM_LOGINFAIL, nSessionID}, stSCAddr);
            return;
        }

        if(pDBHDR->RowCount() < 1){
            g_MonoServer->AddLog(LOGTYPE_INFO, "no guid created for this account: (%s:%s)", szID.c_str(), szPWD.c_str());
            SyncDriver().Forward({SM_LOGINFAIL, nSessionID}, stSCAddr);
            return;
        }

        // structure of database:
        // (id, pwd) -> fld_id
        // fld_id    -> fld_guid
        // fld_guid  -> everything of this char object

        // ok now we find the record coresponding to the id
        AMLoginQueryDB stAMLQDB;

        // 1. session
        stAMLQDB.SessionID = nSessionID;

        pDBHDR->Fetch();

        // 2. needed information to create co
        stAMLQDB.GUID  = std::atoi(pDBHDR->Get("fld_guid"));
        stAMLQDB.MapID = std::atoi(pDBHDR->Get("fld_mapid"));
        stAMLQDB.MapX  = std::atoi(pDBHDR->Get("fld_mapx"));
        stAMLQDB.MapY  = std::atoi(pDBHDR->Get("fld_mapy"));

        // 3. additional information, we can retrieve it later
        stAMLQDB.Level     = std::atoi(pDBHDR->Get("fld_level"));
        stAMLQDB.JobID     = std::atoi(pDBHDR->Get("fld_jobid"));
        stAMLQDB.Direction = std::atoi(pDBHDR->Get("fld_direction"));

        SyncDriver().Forward({MPK_LOGINQUERYDB, stAMLQDB}, stSCAddr);
    };

    extern ThreadPN *g_ThreadPN;
    g_ThreadPN->Add(fnDBOperation);
}
