/*
 * =====================================================================================
 *
 *       Filename: monsterop.cpp
 *        Created: 05/03/2016 21:49:38
 *  Last Modified: 04/01/2017 00:48:36
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

#include "monster.hpp"
#include "actorpod.hpp"
#include "mathfunc.hpp"
#include "monoserver.hpp"

void Monster::On_MPK_METRONOME(const MessagePack &, const Theron::Address &)
{
    Update();
}

void Monster::On_MPK_PULLCOINFO(const MessagePack &rstMPK, const Theron::Address &)
{
    AMPullCOInfo stAMPCOI;
    std::memcpy(&stAMPCOI, rstMPK.Data(), sizeof(stAMPCOI));

    ReportCORecord(stAMPCOI.SessionID);
}
